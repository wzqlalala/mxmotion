#include "mIMxFile.h"
//Qt
#include <QFileInfoList>
#include <QCoreApplication>
//非结构质量
#include "mNonStructQualityManager.h"
#include "mNonStructuralQuality.h"
#include <QDebug>
#include <QMutexLocker>
#include <QThreadPool>
#include <QString>
#include <QEventLoop>
#include <QMultiHash>
#include <QPair>
#include <QFileInfo>
#include <QString>
#include <QMatrix4x4>

//离散场
#include "mFieldManager.h"
#include "mDiscreteField.h"
#include "mAnalysisField.h"

//PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mAppConfiguration.h"

//Function
#include "mElementFunction.h"
#include "mGlobalSignals.h"
#include "mLanguageVarible.h"
#include "mVariableFunction.h"
#include "mPhysicalFieldEnum.h"

//Common signals
#include "mMechGlobalSignals.h"

//模型修复
#include "mEleSetCtrl.h"
#include "mModelFixManager.h"

//BasicData
#include "mMeshData.h"
#include "mPartDataStruct.h"
#include "mParseMeshData.h"
#include "mMapRelationManager.h"

//材料
#include "mMaterialBase.h"
#include "mMaterialManager.h"

//集合
#include "mSetManager.h"
#include "mNode.h"
#include "mElementSet.h"
#include "mElementSurface.h"
#include "mMixSet.h"
#include "mFrequencyPoint.h"
#include "mDOFNode.h"
#include "mPanel.h"
#include "mNodeFace.h"
#include "mSurfaceAnalysis.h"

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
#include "mPropertyBase.h"
#include "mGasket.h"
#include "mGapuni.h"
#include "mMacroEquivalent.h"

//分析工况
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitLinearBuckling.h"
#include "mCaseImplicitModal.h"
#include "mCaseImplicitNonLinearArc.h"
#include "mCaseImplicitNonLinearDynamic.h"
#include "mCaseImplicitNonLinearStatic.h"
#include "mCaseImplicitSteadyHeat.h"
#include "mCaseImplicitTransient.h"
#include "mCaseImplicitTransientHeat.h"
#include "mCaseImplicitStructuralCouplingHeat.h"
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

//载荷
#include "mLoadForce.h"
#include "mLoadGravity.h"
#include "mLoadPressure.h"
#include "mLoadHeatFlux.h"
#include "mLoadRadiation.h"
#include "mLoadConvection.h"
#include "mLoadTransient.h"
#include "mLoadFreqdomain.h"
#include "mLoadCombined.h"
#include "mLoadSpectrum.h"
#include "mLoadAcousticConcForce.h"
#include "mLoadAcousticFrequencyDomain.h"
#include "mLoadBolt.h"
#include "mLoadRotationalACC.h"
#include "mLoadRotationalSpeed.h"
#include "mRotatePhysicalPower.h"
#include "mRotatePhysicalPower.h"
#include "mLoadSurface.h"
#include "mLoadEdge.h"

//边界条件
#include "mBoundaryFixed.h"
#include "mBoundaryDisplacement.h"
#include "mBoundaryTemp.h"
#include "mBoundaryMove.h"
#include "mBoundaryTempNew.h"
#include "mBoundaryVirtual.h"
#include "mBoundarySuperelement.h"

//预定义场
#include "mPredefinedFieldInitTemp.h"
#include "mPredefinedFieldTempLoad.h"
#include "mPredefinedFieldManager.h"

//连接
#include "mConnectorRigid.h"
#include "mConnectorFlexible.h"
#include "mConnectorBind.h"
#include "mConnectorMPC.h"
#include "mConnectorDisperse.h"
#include "mConnectorBolt.h"
#include "mConnectorDistributedCoupling.h"
#include "mConnectorBearingUnits.h"
#include "mUnitIoCodec.h"

//连接副
#include "mConnectingDeputy.h"
#include "mConnectorFastener.h"

//坐标系
#include "mCoordinateBase.h"
#include "mCoordinateManager.h"

//单元
#include "mElementBase.h"
#include "mDataElement.h"

//截面形状
#include "mSectionShapeBase.h"
#include "mSectionShapeManager.h"
#include "H_SectionVisual.h"
#include "L_SectionVisual.h"
#include "I_SectionVisual.h"
#include "I1_SectionVisual.h"
#include "T_SectionVisual.h"
#include "T1_SectionVisual.h"
#include "T2_SectionVisual.h"
#include "Z_SectionVisual.h"
#include "Box1_SectionVisual.h"
#include "Box_SectionVisual.h"
#include "Channel_SectionVisual.h"
#include "Channel1_SectionVisual.h"
#include "Channel2_SectionVisual.h"
#include "Annular_SectionVisual.h"
#include "Circle_SectionVisual.h"
#include "Cross_SectionVisual.h"
#include "Hat_SectionVisual.h"
#include "Rectangle_SectionVisual.h"
#include "mCustomSectionShape.h"

//结构优化
#include "mStructOption.h"
#include "mModelData.h"
#include "mOptimizationDesignData.h"
#include "mOptimizationGoalData.h"
#include "mOptimizationConstraintData.h"

//全局参数
#include "mGlobalParam.h"
#include "mGenericParamData.h"
#include "mGenericAssignData.h"
#include "mGenericMatrixData.h"
#include "mPeakFilterData.h"
#include "mGlobalOutputControlData.h"

//采样频率点
#include "mFreqSamplPointBase.h"

//层合结构
#include "mLaminateBase.h"
#include "mLaminateManager.h"
#include "mSglLayerPlate.h"
#include "mGlobalSglLam.h"
#include "mLamPlate.h"
#include "mOtherFunction.h"

#include "mIOFileManager.h"

#define readFileTimeRatio 90

using namespace std;
using namespace MechModelData;
using namespace MxFunctions;
using namespace MechPreData;
using namespace MechMaterialData;
using namespace MechSetData;
using namespace MechPropertyData;
using namespace MechCaseData;
using namespace MechContactData;
using namespace MechConnectorData;
using namespace MechCoordinateData;
using namespace MechSectionShapeData;
using namespace MechElementData;
using namespace MechMapRelation;
using namespace MechStructOptionData;
using namespace MechGlobalParamData;
using namespace MechFunctionData;
using namespace MechDiscreteFieldData;
using namespace MechModelFixData;
namespace MechIOFile
{
    mIMxFile::~mIMxFile()
    {

    }


    void mIMxFile::scanMxFile(const QString& filename)
    {
        _modeldata = getCVM->getModelData();
        if (_modeldata == nullptr)
        {
            return;
        }

        _meshData = _modeldata->getMeshData();
        _elementEndLine = 0;
        _curnodenum = _meshData->_nodedata.nodeamount;

        QFile _file(filename);
        if (_file.open(QIODevice::ReadOnly | QIODevice::Text))//以读的方式打开文件
        {
            int tempnode = 0;
            QMap<QString, QVector<QPair<QString, int>>> tempele;
            while (!_file.atEnd())
            {
                if (currentline.startsWith("*Node", Qt::CaseInsensitive))
                {
                    while (!_file.atEnd())
                    {
                        currentline = _file.readLine().simplified();
                        if (currentline.contains("*"))
                        {
                            break;
                        }
                        else if (currentline.isEmpty())
                        {
                            continue;
                        }
                        tempnode++;
                    }
                }
                else if (currentline.startsWith("*Element", Qt::CaseInsensitive))
                {
                    MxElementMessage mess = GetElementMessage(currentline);
                    if (isSpecialElement(mess.elementType))
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }
                    int typecount = 0;
                    while (!_file.atEnd())
                    {
                        currentline = _file.readLine().simplified();
                        if (currentline.contains("*"))
                        {
                            break;
                        }
                        else if (currentline.isEmpty())
                        {
                            continue;
                        }
                        typecount++;
                    }
                    QPair<QString, int> elepair(mess.elementString, typecount);
                    QVector<QPair<QString, int>> elepair_temp;
                    if (tempele.find(mess.partName) != tempele.end())
                    {
                        elepair_temp = tempele.value(mess.partName);
                    }
                    elepair_temp.push_back(elepair);
                    tempele.insert(mess.partName, elepair_temp);
                    _elementEndLine = _file.pos();
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }


            if (_meshData->_nodedata.nodeamount > 0)
            {
                int nodeNum = _meshData->_nodedata.nodeamount + tempnode;
                double* NodeXYZ = (double*)malloc(nodeNum * 3 * sizeof(double));
                int* NodeRealids = (int*)malloc(nodeNum * sizeof(int));
                memcpy(NodeRealids, _meshData->_nodedata.realids, _meshData->_nodedata.nodeamount * sizeof(int));
                memcpy(NodeXYZ, _meshData->_nodedata.xyz, _meshData->_nodedata.nodeamount * 3 * sizeof(double));
                free(_meshData->_nodedata.xyz);
                free(_meshData->_nodedata.realids);
                _meshData->_nodedata.nodeamount = nodeNum;
                _meshData->_nodedata.xyz = NodeXYZ;
                _meshData->_nodedata.realids = NodeRealids;
            }
            else
            {
                _meshData->_nodedata.nodeamount = tempnode;
                _meshData->_nodedata.xyz = (double*)malloc(tempnode * 3 * sizeof(double));
                _meshData->_nodedata.realids = (int*)malloc(tempnode * sizeof(int));
            }
            //解析单元汇总信息
            for (auto it = tempele.begin(); it != tempele.end(); ++it)
            {
                int id = _meshData->getMaxPartID() + 1;
                mPartData* pd = new mPartData(it.key(), id);
                _meshData->_partdata.insert(id, pd);
                int& etn = pd->_parteledata.elementtypenum;
                int* et = pd->_parteledata.elementtype;
                int* enc = pd->_parteledata.elementnodecount;

                int elenodeamount = 0;
                int elemount = 0;
                for (auto eletTypeToCount : it.value())
                {
                    ElementType etenum = mElementFunction::getElementType(eletTypeToCount.first);
                    et[etn] = (int)(etenum);
                    int encount = mElementFunction::elementNodeCount(etenum);
                    enc[etn * 2] = encount;
                    enc[etn * 2 + 1] = eletTypeToCount.second;
                    etn++;

                    elemount += eletTypeToCount.second;
                    elenodeamount += encount * eletTypeToCount.second;
                }

                pd->_parteledata.nodeids = (int*)malloc(elenodeamount * sizeof(int));
                pd->_parteledata.realids = (int*)malloc(elemount * sizeof(int));
            }
        }
        _file.close();
    }

    bool mIMxFile::readMxFile(const QString& filename)
    {
        currentline = "";

        _lastMaxMeshID = _meshData->getMaxEleId();
        _lastMaxNodeID = _meshData->getMaxNodeId();

        QTextCodec* codeC = QTextCodec::codecForName("UTF-8");
        _file.setFileName(filename);
        _filename = filename;
        if (_file.open(QIODevice::ReadOnly | QIODevice::Text))//以读的方式打开文件
        {
            if (_isMeshFile)
            {
                _fileSize = _elementEndLine;
            }
            else
            {
                _fileSize = _file.size();
            }

            while (!_file.atEnd())//文件没到末尾就执行循环体内容
            {
                if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
                {
                    _file.close();
                    return false;
                }

                setProgressBarValue(_file.pos());

                if (_isMeshFile && (!currentline.contains("*node", Qt::CaseInsensitive) && !currentline.contains("*Element", Qt::CaseInsensitive)))
                {
                    QByteArray line = _file.readLine();
                    currentline = codeC->toUnicode(line);
                    continue;
                }

                //读取节点信息
                if (currentline.startsWith("*node", Qt::CaseInsensitive))
                {
                    mGlobalSignals::getInstance()->outputMessageSig(mLanguageVarible::FindLanguageValue("正在读取节点数据..."));
                    ReadNodeData();
                    mGlobalSignals::getInstance()->outputMessageSig(mLanguageVarible::FindLanguageValue("节点读取完成"));
                }
                else if (currentline.startsWith("*Element", Qt::CaseInsensitive))
                {
                    readElementData();
                    if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
                    {
                        _file.close();
                        return false;
                    }
                    else if (_elementEndLine == _file.pos())
                    {
                        mGlobalSignals::getInstance()->outputMessageSig(mLanguageVarible::FindLanguageValue("数据处理中..."));
                        //整理数据
                        mParseMeshData* pmd = new mParseMeshData(_meshData);
                        pmd->ParseMesh();
                        delete pmd;
                        mGlobalSignals::getInstance()->outputMessageSig(mLanguageVarible::FindLanguageValue("数据处理完成"));
                        if (_isMeshFile)
                        {
                            break;
                        }
                    }
                }
                //读取材料信息
                else if (currentline.startsWith("*material", Qt::CaseInsensitive))
                {
                    ReadMaterialData();
                }
                //读取截面属性信息
                else if (currentline.startsWith("*section", Qt::CaseInsensitive))
                {
                    ReadPropertyData();
                }
                //读取分析工况
                else if (currentline.startsWith("*subcase", Qt::CaseInsensitive))
                {
                    ReadCaseData();
                }
                //读取结构优化
                else if (currentline.startsWith("*optimization", Qt::CaseInsensitive))
                {
                    ReadStructOptionData();
                }
                //读取集合
                else if (currentline.startsWith("*set", Qt::CaseInsensitive))
                {
                    ReadSetData();
                }
                //读取非结构质量
                else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
                {
                    ReadNonStructQualityData();
                }
                //读取离散场
                else if (currentline.startsWith("*distribution", Qt::CaseInsensitive))
                {
                    readDiscreteField();
                }
                //读取分析场
                else if (currentline.startsWith("*analysisfield", Qt::CaseInsensitive))
                {
                    readAnalysisField();
                }
                //读取过盈接触
                else if (currentline.startsWith("*contactinterference", Qt::CaseInsensitive))
                {
                    ReadContactinterference();
                }
                //读取接触
                else if (currentline.startsWith("*contact", Qt::CaseInsensitive))
                {
                    ReadContactData();
                }
                //读取周期性边界
                else if (currentline.startsWith("*periodic", Qt::CaseInsensitive))
                {
                    readPeriodicBoundaryData();
                }
                //读取载荷
                else if (currentline.startsWith("*nodalload", Qt::CaseInsensitive) || currentline.startsWith("*bodyload", Qt::CaseInsensitive)
                    || currentline.startsWith("*surfaceload", Qt::CaseInsensitive) || currentline.startsWith("*nodalflux", Qt::CaseInsensitive) || currentline.startsWith("*tload", Qt::CaseInsensitive)
                    || currentline.startsWith("*rload", Qt::CaseInsensitive) || currentline.startsWith("*rload1", Qt::CaseInsensitive) || currentline.startsWith("*dload", Qt::CaseInsensitive)
                    || currentline.startsWith("*acsrce", Qt::CaseInsensitive) || currentline.startsWith("*sload", Qt::CaseInsensitive)
                    || currentline.startsWith("*rload", Qt::CaseInsensitive) || currentline.startsWith("*rload1", Qt::CaseInsensitive)
                    || currentline.startsWith("*dload", Qt::CaseInsensitive) || currentline.startsWith("*ds", Qt::CaseInsensitive)
                    || currentline.startsWith("*omega", Qt::CaseInsensitive) || currentline.startsWith("*domega", Qt::CaseInsensitive)
                    || currentline.startsWith("*edgeload", Qt::CaseInsensitive)
                    )
                {
                    ReadLoadData();
                }
                //读取边界条件
                else if (currentline.startsWith("*spc", Qt::CaseInsensitive) || currentline.startsWith("*suport1", Qt::CaseInsensitive)
                    || currentline.startsWith("*constraint", Qt::CaseInsensitive))
                {
                    readBoundary();
                }
                else if (currentline.startsWith("*coordinatesystem", Qt::CaseInsensitive))
                {
                    readCoordinatesystem();
                }
                //读取连接
                else if (currentline.startsWith("*connector_part", Qt::CaseInsensitive) || currentline.startsWith("*connector", Qt::CaseInsensitive))
                {
                    readConnector();
                }
                //读离散刚体与解析刚体
                else if (currentline.startsWith("*rigidbody", Qt::CaseInsensitive))
                {
                    readDisperse();
                }
                //读螺栓约束
                else if (currentline.startsWith("*pretension", Qt::CaseInsensitive))
                {
                    readPretension();
                }
                //读取采样频率点
                else if (currentline.startsWith("*freqpoint", Qt::CaseInsensitive))
                {
                    readFreqSamplPointData();
                }
                //读取层合结构
                else if (currentline.startsWith("*layer", Qt::CaseInsensitive) || currentline.startsWith("*stack", Qt::CaseInsensitive))
                {
                    readLaminateData();
                }
                //读取超单元调用
                else if (currentline.startsWith("*assign", Qt::CaseInsensitive))
                {
                    readGenericAssignData();
                }
                //读取超单元参数
                else if (currentline.startsWith("*CMSMethod", Qt::CaseInsensitive))
                {
                    readGenericParamData();
                }
                //读取全局参数
                else if (currentline.startsWith("*param", Qt::CaseInsensitive))
                {
                    readParam();
                }
                //读取传递路径贡献量
                else if (currentline.startsWith("*path", Qt::CaseInsensitive))
                {
                    readTPContriData();
                }
                //读取函数
                else if (currentline.startsWith("*function", Qt::CaseInsensitive))
                {
                    readFunctionData();
                }
                //读取全局输出控制
                else if (currentline.startsWith("*globaloutput", Qt::CaseInsensitive))
                {
                    readGlobalOutputControl();
                }
                //读取峰值筛选
                else if (currentline.startsWith("*peakout", Qt::CaseInsensitive))
                {
                    readPeakFilterData();
                }
                //读取声固耦合搜索参数
                else if (currentline.startsWith("*fsi_pair", Qt::CaseInsensitive))
                {
                    readASCouplingSearchData();
                }
                //读取模型修复
                else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
                {
                    readModelChangeData();
                }
                //读取初始温度预定义场
                else if (currentline.startsWith("*ibc", Qt::CaseInsensitive))
                {
                    readInitTempData();
                }
                //读取温度载荷预定义场
                else if (currentline.startsWith("*temp", Qt::CaseInsensitive) && !currentline.startsWith("*temp_load", Qt::CaseInsensitive))
                {
                    readTempLoadData();
                }
                else
                {
                    QByteArray line = _file.readLine();
                    currentline = codeC->toUnicode(line).simplified();
                }
            }
        }
        _file.close();

        //解析映射关系
        analysisMapRelation();

        return true;
    }


    void mIMxFile::setIsMeshFile()
    {
        _isMeshFile = true;
    }

    std::shared_ptr<MechMaterialData::mMaterialBase> mIMxFile::ReadCustomMaterialData(const QString& filename)
    {
        QString appDir = QCoreApplication::applicationDirPath();
        QString matFile = appDir + "/CustomMaterials/" + filename +".mat";
        _file.setFileName(matFile);
        if (!_file.exists()) {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("自定义材料库文件不存在"));
            return NULL;
        }
        if (_file.open(QIODevice::ReadOnly | QIODevice::Text))//以读的方式打开文件
        {
            currentline = _file.readLine();
            QString materialname = getKeyValue(currentline, "name", "=");
            QString materialtype = getKeyValue(currentline, "type", "=");
            //获取与id相关的颜色
            int nColor = PartColorLoop.size();
            QVector4D vector4d = PartColorLoop[5 % nColor];
            QColor color = QColor(vector4d.x() * 255, vector4d.y() * 255, vector4d.z() * 255, vector4d.w() * 255);
            shared_ptr<MechMaterialData::mMaterialBase> mat = make_shared<MechMaterialData::mMaterialBase>(-1, color, NONEMATERIALTYPE, materialname);
            if (!materialtype.compare("isolinearelastic", Qt::CaseInsensitive))
            {
                readIsolinearelastic(mat);
            }
            else if (!materialtype.compare("ortholinearelastic2d", Qt::CaseInsensitive))
            {
                readOrtholinearelastic2d(mat);
            }
            else if (!materialtype.compare("ortholinearelastic3d", Qt::CaseInsensitive))
            {
                readOrtholinearelastic3d(mat);
            }
            else if (!materialtype.compare("anisotropiclinearelastic2d", Qt::CaseInsensitive))
            {
                readAnisotropiclinearelastic2d(mat);
            }
            else if (!materialtype.compare("anisotropiclinearelastic3d", Qt::CaseInsensitive))
            {
                readAnisotropiclinearelastic3d(mat);
            }
            /*else if (!materialtype.compare("heat", Qt::CaseInsensitive))
            {
                readHeatMat(mat);
            }*/
            else if (!materialtype.compare("heat", Qt::CaseInsensitive))
            {
                readHeatMat(mat);
            }
            else if (!materialtype.compare("chaboche", Qt::CaseInsensitive))
            {
                readChabocheMat(mat);
            }
            else if (!materialtype.compare("acoustic_mat", Qt::CaseInsensitive))
            {
                readaCousticMat(mat);
            }
            else if (!materialtype.compare("deformationplasticity", Qt::CaseInsensitive))
            {
                readDeformationplasticityMat(mat);
            }
            else if (!materialtype.compare("elastoplastic", Qt::CaseInsensitive))
            {
                readElastoplasticMat(mat);
            }
            else if (!materialtype.compare("GasketBehavior", Qt::CaseInsensitive))
            {
                readShimMat(mat);
            }
            else if (!materialtype.compare("viscoplastic", Qt::CaseInsensitive))
            {
                readViscoplasticityMat(mat);
            }
            else if (!materialtype.compare("viscoelastic", Qt::CaseInsensitive))
            {
                readViscoelasticMat(mat);
            }
            else if (!materialtype.compare("neohookean2", Qt::CaseInsensitive))
            {
                readNeoHookean2Mat(mat);
            }
            else if (!materialtype.compare("mooneyrivlin", Qt::CaseInsensitive))
            {
                readMooneyRivlinMat(mat);
            }
            else if (!materialtype.compare("arrudaboyce", Qt::CaseInsensitive))
            {
                readArrudaBoyceMat(mat);
            }
            else if (!materialtype.compare("yeoh", Qt::CaseInsensitive))
            {
                readYeohMat(mat);
            }
            else if (!materialtype.compare("ogden", Qt::CaseInsensitive))
            {
                readOgdenMat(mat);
            }
            else if (!materialtype.compare("hyperfoam", Qt::CaseInsensitive))
            {
                readHyperforMat(mat);
            }
            else if (!materialtype.compare("polynomial", Qt::CaseInsensitive))
            {
                readPolynomialMat(mat);
            }
            else if (!materialtype.compare("phasematerial", Qt::CaseInsensitive))
            {
                readBreakMat(mat);
            }
            else if (!materialtype.compare("JohnsonCookPlastic", Qt::CaseInsensitive))
            {
                readJohnsonCookMat(mat);
            }
            else if (!materialtype.compare("sheardamage", Qt::CaseInsensitive))
            {
                readShearDamageMat(mat);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("材料 %1  类型不支持！").arg(materialname));
                currentline = _file.readLine().simplified();
                return NULL;
            }

            while (!_file.atEnd())
            {
                //密度
                if (currentline.startsWith("*density", Qt::CaseInsensitive))
                {
                    readDensity(mat);
                }
                else if (currentline.startsWith("*materialdamping", Qt::CaseInsensitive))
                {
                    readMaterialDamping(mat);
                }
                else if (currentline.startsWith("*expansion", Qt::CaseInsensitive))
                {
                    readExpansion(mat);
                }
                else if (currentline.startsWith("*heat", Qt::CaseInsensitive))
                {
                    readHeat(mat);
                }
                else if (currentline.startsWith("*creep", Qt::CaseInsensitive))
                {
                    readCreep(mat);
                }
                else if (currentline.startsWith("*cyclichardening", Qt::CaseInsensitive))
                {
                    readCyclicHardeningWithTable(mat);
                }
                else if (currentline.startsWith("*elastic", Qt::CaseInsensitive))
                {
                    readFlexMat(mat);
                }
                else if (currentline.startsWith("*plastic", Qt::CaseInsensitive))
                {
                    QString number = getKeyValue(currentline, "number", "=");
                    QString hardening = getKeyValue(currentline, "hardening", "=");
                    if (!number.isEmpty())
                    {
                        readPlasticityWithOrderMat(mat);
                    }
                    else
                    {
                        if (hardening.isEmpty())
                            readPlasticityJohnsonMat(mat);
                        else
                            readPlasticityComboMat(mat);
                    }
                }
                else if (currentline.startsWith("*ratedependent", Qt::CaseInsensitive))
                {
                    readRateDependentMat(mat);
                }
                else if (currentline.startsWith("*sheardamage", Qt::CaseInsensitive))
                {
                    readShearDamage(mat);
                }
                else if (currentline.startsWith("*damageevolution", Qt::CaseInsensitive))
                {
                    readDamageEvolution(mat);
                }
                else if (currentline.startsWith("*GasketThicknessBehavior", Qt::CaseInsensitive))
                {
                    readThicknessBehaviorMat(mat);
                }
                else if (currentline.startsWith("*GasketElasticity", Qt::CaseInsensitive))
                {
                    QString component = getKeyValue(currentline, "component", "=");
                    if (!component.compare("membrane", Qt::CaseInsensitive))
                    {
                        readFilmBehaviorMat(mat);
                    }
                    else if (!component.compare("stresstiffness", Qt::CaseInsensitive))
                    {
                        readCutBehaviorMat(mat);
                    }
                }
                else if (currentline.startsWith("*failstrain", Qt::CaseInsensitive))
                {
                    readFailureEffect(mat);
                }
                else if (currentline.startsWith("*failstress", Qt::CaseInsensitive))
                {
                    readFailureStress(mat);
                }
                else if (currentline.startsWith("*energydamage", Qt::CaseInsensitive))
                {
                    ReadEnergyDamager(mat);
                }
                else if (currentline.startsWith("*hashindamage", Qt::CaseInsensitive))
                {
                    ReadHashIndamage(mat);
                }
                else if (currentline.startsWith("*phaseparameter", Qt::CaseInsensitive))
                {
                    readPhaseParam(mat);
                }
                else
                {
                    break;
                }
            }
            _file.close();
            return mat;
        }
    }

    MxElementMessage mIMxFile::GetElementMessage(QString list)
    {
        QString elementString = getKeyValue(list, "type", "=").toUpper();
        ElementType elementType;
        if (elementString == "BEARING")
        {
            elementType = ElementType::BEARING;
        }
        else {
            elementType = mElementFunction::getElementType(elementString);
        }

        QString partname = getKeyValue(list, "part", "=");
        if (!partname.isEmpty())
        {
            partname = ExistRenamePhysicalName(Physical_Part, partname);
        }

        //获取截面属性名称
        QString sectionname = getKeyValue(list, "section", "=");
        if (!sectionname.isEmpty())
        {
            sectionname = ExistRenamePhysicalName(Physical_Property, sectionname);
        }

        MxElementMessage message;
        message.elementType = elementType;
        message.partName = partname;
        message.sectionname = sectionname;
        message.elementString = elementString;
        return message;
    }

    bool mIMxFile::isSpecialElement(ElementType type)
    {
        switch (type)
        {
        case CONN3D2:
            return true;
        case BEARING:
            return true;
        default:
            break;
        }

        return false;
    }

    QString mIMxFile::getKeyValue(const QString& str_line, const QString& key)
    {
        QString name = "";

        int pos = str_line.indexOf(key);
        if (pos > -1)
        {
            pos += key.size();
            int pos_end = pos + 1;
            if (str_line[pos] == '\"')
            {
                while (str_line[pos_end] != '\"')
                {
                    pos_end++;
                }
                name = str_line.mid(pos + 1, pos_end - pos - 1);
            }
            else
            {
                while (pos_end < str_line.size() && str_line[pos_end] != ',')
                {
                    pos_end++;
                }
                name = str_line.mid(pos, pos_end - pos);
            }
        }
        return name;
    }

    QString mIMxFile::getKeyValue(const QString& str_line, const QString& key, const QString& interval)
    {
        const QString pattern =
            "(?:^|[\\s,;])\\s*\\b%1\\b\\s*%2\\s*(?:\"([^\"]*)\"|([^,;]*))";

        const QRegularExpression re(
            pattern
            .arg(QRegularExpression::escape(key),
                QRegularExpression::escape(interval)),
            QRegularExpression::CaseInsensitiveOption
        );

        const auto m = re.match(str_line);
        if (!m.hasMatch()) return {};

        const QString value = !m.captured(1).isEmpty() ? m.captured(1) : m.captured(2);
        return value.trimmed();
    }

    QString mIMxFile::getQStringListIndexValue(const QStringList& list, const int& index)
    {
        if (list.size() <= index)
            return QString();
        QStringList strList = list[index].simplified().split("\"");
        if (strList.size() == 1)
            return list[index].simplified();
        else if (strList.size() == 3)
            return strList[1].simplified();

        return QString();
    }


    void mIMxFile::ReadNodeData()
    {
        double* xyz = _meshData->_nodedata.xyz;
        int* ids = _meshData->_nodedata.realids;
        QHash<int, int>& nodeidmap = _meshData->_nodedata.nodeidmap;
        while (!_file.atEnd())
        {
            setProgressBarValue(_file.pos());
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }
            _linelist = currentline.split(",");
            if (_linelist.size() >= 4)
            {
                int id = _linelist.at(0).toInt() + _lastMaxNodeID;
                double x = _linelist.at(1).toDouble();
                double y = _linelist.at(2).toDouble();
                double z = _linelist.at(3).toDouble();

                //单位制换算：文件值(当前界面单位制) → 内部mm-t-N-s（长度）；
                //带参考/显示坐标系列的节点先存原值，由nodeCoordMap按坐标系类型换算
                //（柱坐标下y列为角度，属文件格式固定语义，不换算）
                if (_linelist.size() < 5)
                {
                    x = mUnitIoCodec::decode(MxFunctions::Unit::Length, x);
                    y = mUnitIoCodec::decode(MxFunctions::Unit::Length, y);
                    z = mUnitIoCodec::decode(MxFunctions::Unit::Length, z);
                }

                xyz[_curnodenum * 3] = x;
                xyz[_curnodenum * 3 + 1] = y;
                xyz[_curnodenum * 3 + 2] = z;
                ids[_curnodenum] = id;
                nodeidmap.insert(id, _curnodenum);
                _curnodenum++;

                if (_linelist.size() >= 5)
                {
                    _nodeCoordinateData[id] = { getQStringListIndexValue(_linelist, 4), getQStringListIndexValue(_linelist, 5) };
                }
            }
        }
    }

    void mIMxFile::readElementData()
    {
        MxElementMessage mess = GetElementMessage(currentline);
        QString pname = mess.partName;
        if (isSpecialElement(mess.elementType))
        {
            readSpecialElement(mess);
            return;
        }

        mPartData* pd = _meshData->getPartData(pname);
        if (pd == nullptr)
        {
            return;
        }
        int partID = pd->getPartID();
        appendMapPhysicalStruct(Physical_Part, partID, Physical_Property, mess.sectionname);
        long long int& _ele_node_count = pd->_parteledata._ele_node_count;
        int& elenum = pd->_parteledata.currentelenum;
        int* nodeids = pd->_parteledata.nodeids;
        QHash<int, int>& elementidmap = pd->_parteledata.elementidmap;
        QHash<int, QPair<int, int>>& elenodeposmap = pd->_parteledata.elenodeposmap;
        int* elementids = pd->_parteledata.realids;
        int encount = mElementFunction::elementNodeCount(mess.elementType);

        mGlobalSignals::getInstance()->outputMessageSig(mLanguageVarible::FindLanguageValue("正在读取单元%1").arg(mess.elementString));
        while (!_file.atEnd())
        {
            setProgressBarValue(_file.pos());
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }
            QStringList list_line = currentline.split(",");
            if (list_line.size() > 1)
            {
                //存储单元信息
                int id = list_line.at(0).toInt() + _lastMaxMeshID;
                _elementIDToPartID[id] = partID;
                elementidmap.insert(id, elenum);
                elementids[elenum] = id;
                elenum++;
                elenodeposmap.insert(id, QPair<int, int>(_ele_node_count, encount));

                //获取节点
                for (int i = 0; i < encount; ++i)
                {
                    int nidi = list_line.at(i + 1).toInt() + _lastMaxNodeID;
                    nodeids[_ele_node_count] = nidi;
                    _ele_node_count++;
                    _meshData->addNodeFindElementData(pd, nidi, id);
                }
                setElementExtraData(id, mess.elementType, currentline);
            }
        }
    }
    void mIMxFile::readSpecialElement(MxElementMessage mess)
    {
        switch (mess.elementType)
        {
        case CONN3D2:
        {
            MechConnectorData::mDeputyManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);
            QString name = ExistRenamePhysicalName(Physical_Deputy, mess.sectionname);
            std::shared_ptr<MechConnectorData::mConnectingDeputy> deputyData = std::make_shared<MechConnectorData::mConnectingDeputy>(manager.createID(), Deputy_Connector, name);
            getCVM->getModelData()->addPhysicalFieldP(deputyData);

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.contains("*"))
                {
                    break;
                }
                else if (currentline.isEmpty())
                {
                    continue;
                }
                _linelist = currentline.split(",");
                if (_linelist.size() >= 3)
                {
                    int node1 = getQStringListIndexValue(_linelist, 1).toInt() + _lastMaxNodeID;
                    int node2 = getQStringListIndexValue(_linelist, 2).toInt() + _lastMaxNodeID;
                    deputyData->refNodeIds_.push_back(node1);
                    deputyData->movNodeIds_.push_back(node2);
                }
            }
            break;
        }
        case BEARING:
        {
            MechConnectorData::mConnectorManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector);
            QString name = ExistRenamePhysicalName(Physical_Connector, mess.sectionname);
            int connectID = manager.createID();
            std::shared_ptr<MechConnectorData::mConnectorBearingUnits> connect = std::make_shared<MechConnectorData::mConnectorBearingUnits>(connectID, Connector_BearingUnits, name);
            getCVM->getModelData()->addPhysicalFieldP(connect);
            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            int node1 = getQStringListIndexValue(_linelist, 1).toInt() + _lastMaxNodeID;
            int node2 = getQStringListIndexValue(_linelist, 2).toInt();
            if (node2 == 0)
            {
                connect->type_ = 0;
            }
            else
            {
                connect->type_ = 1;
                node2 += _lastMaxNodeID;
            }
            connect->node1_.first = MxFunctions::PickDataType::Node;
            connect->node1_.second.result_id = node1;
            connect->node2_.first = MxFunctions::PickDataType::Node;
            connect->node2_.second.result_id = node2;
            QString coor1 = getQStringListIndexValue(_linelist, 3).trimmed();
            QString coor2 = getQStringListIndexValue(_linelist, 4).trimmed();
            appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Coordinate, coor1);
            appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Coordinate, coor2, MapRelation_Coord2);
            break;
        }

        default:
            break;
        }
    }
    void mIMxFile::ReadStructOptionData()
    {
        if (_modeldata->getStructOptionBase())
        {
            currentline = _file.readLine().simplified();
            return;
        }

        MechStructOptionData::mStructOption* opt = _modeldata->createStructOption();

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*topoparam", Qt::CaseInsensitive))
            {
                ReadStructOptionTopopara();
            }
            else if (currentline.startsWith("*designvar", Qt::CaseInsensitive))
            {
                ReadStructOptionDesign();
            }
            else if (currentline.startsWith("*dconstrain", Qt::CaseInsensitive))
            {
                ReadStructOptionConstraint();
            }
            else if (currentline.startsWith("*objective", Qt::CaseInsensitive))
            {
                ReadStructOptionObjective();
            }
            else if (currentline.startsWith("*matintertype", Qt::CaseInsensitive))
            {
                opt->_mStructOptionData._isMatintertype = true;
                currentline = _file.readLine().simplified();
                opt->_mStructOptionData._matintertype = currentline;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*restart", Qt::CaseInsensitive))
            {
                QString iteration = getKeyValue(currentline, "iteration", "=");
                QString filepath = getKeyValue(currentline, "filepath", "=");
                QFileInfo fileInfo(filepath);
                if (!fileInfo.isAbsolute())
                {
                    QFileInfo baseFileInfo(_filename);
                    filepath = baseFileInfo.absolutePath() + "/" + filepath;
                }
                opt->_mStructOptionData._isRestart = true;
                opt->_mStructOptionData._iteration = iteration;
                opt->_mStructOptionData._resStartPath = filepath;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*", Qt::CaseInsensitive))
            {
                break;
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }

    }



    void mIMxFile::ReadCaseData()
    {
        QString casename = getKeyValue(currentline, "name", "=");
        casename = ExistRenamePhysicalName(Physical_Case, casename);
        currentline = _file.readLine().simplified();
        QString type = getKeyValue(currentline, "type", "=");

        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        if (!type.compare("linearstatic", Qt::CaseInsensitive))
        {
            readLinearstatic(casename, caseID);
        }
        else if (!type.compare("nonlinear", Qt::CaseInsensitive))
        {
            readNonlinear(casename, caseID);
        }
        else if (!type.compare("heattransfer", Qt::CaseInsensitive))
        {
            readHeattransfer(casename, caseID);
        }
        else if (!type.compare("modes", Qt::CaseInsensitive)
            || !type.compare("complexmodes1", Qt::CaseInsensitive)
            || !type.compare("rotor_modes", Qt::CaseInsensitive)
            )
        {
            readModesComplexmodes(casename, caseID);
        }
        else if (!type.compare("buckling", Qt::CaseInsensitive))
        {
            readBuckling(casename, caseID);
        }
        else if (!type.compare("random", Qt::CaseInsensitive))
        {
            readRandomVibration(casename, caseID);
        }
        else if (!type.compare("responsespectrum", Qt::CaseInsensitive))
        {
            readResponseSpectrum(casename, caseID);
        }
        else if (!type.compare("frequencyresponse_modal", Qt::CaseInsensitive) || !type.compare("frequencyresponse_direct", Qt::CaseInsensitive))
        {
            readFrequencyresponse(casename, caseID);
        }
        else if (!type.compare("frequencyresponse_modal1", Qt::CaseInsensitive))
        {
            readASCoupling(casename, caseID);
        }
        else if (!type.compare("lineartransient_modal", Qt::CaseInsensitive) || !type.compare("lineartransient_direct", Qt::CaseInsensitive))
        {
            readLineartransient(casename, caseID);
        }
        else if (!type.compare("generic", Qt::CaseInsensitive))
        {
            readGeneric(casename, caseID);
        }
        else
        {
            currentline = _file.readLine().simplified();
        }

    }

    void mIMxFile::ReadMaterialData()
    {
        QString materialname = getKeyValue(currentline, "name", "=");
        materialname = ExistRenamePhysicalName(Physical_Material, materialname);
        QString materialtype = getKeyValue(currentline, "type", "=");

        int matID = _modeldata->getManager<MechMaterialData::mMaterialManager>(Physical_Material).createID();

        //获取与id相关的颜色
        int nColor = PartColorLoop.size();
        QVector4D vector4d = PartColorLoop[(matID - 1) % nColor];
        QColor color = QColor(vector4d.x() * 255, vector4d.y() * 255, vector4d.z() * 255, vector4d.w() * 255);

        shared_ptr<MechMaterialData::mMaterialBase> mat = make_shared<MechMaterialData::mMaterialBase>(matID, color, MaterialType::NONEMATERIALTYPE, materialname);
        _modeldata->addPhysicalFieldP(mat);

        /*mat = getCVM->getModelData()
            ->getManager<MechMaterialData::mMaterialManager>(Physical_Material)
            .getMaterialData(matID);*/

        if (!materialtype.compare("isolinearelastic", Qt::CaseInsensitive))
        {
            readIsolinearelastic(mat);
        }
        else if (!materialtype.compare("ortholinearelastic2d", Qt::CaseInsensitive))
        {
            readOrtholinearelastic2d(mat);
        }
        else if (!materialtype.compare("ortholinearelastic3d", Qt::CaseInsensitive))
        {
            readOrtholinearelastic3d(mat);
        }
        else if (!materialtype.compare("anisotropiclinearelastic2d", Qt::CaseInsensitive))
        {
            readAnisotropiclinearelastic2d(mat);
        }
        else if (!materialtype.compare("anisotropiclinearelastic3d", Qt::CaseInsensitive))
        {
            readAnisotropiclinearelastic3d(mat);
        }
        else if (!materialtype.compare("heat", Qt::CaseInsensitive))
        {
            readHeatMat(mat);
        }
        else if (!materialtype.compare("chaboche", Qt::CaseInsensitive))
        {
            readChabocheMat(mat);
        }
        else if (!materialtype.compare("JohnsonCookPlastic", Qt::CaseInsensitive))
        {
            readJohnsonCookMat(mat);
        }
        else if (!materialtype.compare("acoustic_mat", Qt::CaseInsensitive))
        {
            readaCousticMat(mat);
        }
        else if (!materialtype.compare("deformationplasticity", Qt::CaseInsensitive))
        {
            readDeformationplasticityMat(mat);
        }
        else if (!materialtype.compare("elastoplastic", Qt::CaseInsensitive))
        {
            readElastoplasticMat(mat);
        }
        else if (!materialtype.compare("GasketBehavior", Qt::CaseInsensitive))
        {
            readShimMat(mat);
        }
        else if (!materialtype.compare("viscoplastic", Qt::CaseInsensitive))
        {
            readViscoplasticityMat(mat);
        }
        else if (!materialtype.compare("viscoelastic", Qt::CaseInsensitive))
        {
            readViscoelasticMat(mat);
        }
        else if (!materialtype.compare("neohookean2", Qt::CaseInsensitive))
        {
            readNeoHookean2Mat(mat);
        }
        else if (!materialtype.compare("mooneyrivlin", Qt::CaseInsensitive))
        {
            readMooneyRivlinMat(mat);
        }
        else if (!materialtype.compare("arrudaboyce", Qt::CaseInsensitive))
        {
            readArrudaBoyceMat(mat);
        }
        else if (!materialtype.compare("yeoh", Qt::CaseInsensitive))
        {
            readYeohMat(mat);
        }
        else if (!materialtype.compare("ogden", Qt::CaseInsensitive))
        {
            readOgdenMat(mat);
        }
        else if (!materialtype.compare("hyperfoam", Qt::CaseInsensitive))
        {
            readHyperforMat(mat);
        }
        else if (!materialtype.compare("polynomial", Qt::CaseInsensitive))
        {
            readPolynomialMat(mat);
        }
        else if (!materialtype.compare("phasematerial", Qt::CaseInsensitive))
        {
            readBreakMat(mat);
        }
        else if (!materialtype.compare("sheardamage", Qt::CaseInsensitive))
        {
            readShearDamageMat(mat);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("材料 %1  类型不支持！").arg(materialname));
            currentline = _file.readLine().simplified();
            return;
        }

        while (!_file.atEnd())
        {
            //密度
            if (currentline.startsWith("*density", Qt::CaseInsensitive))
            {
                readDensity(mat);
            }
            else if (currentline.startsWith("*materialdamping", Qt::CaseInsensitive))
            {
                readMaterialDamping(mat);
            }
            else if (currentline.startsWith("*expansion", Qt::CaseInsensitive))
            {
                readExpansion(mat);
            }
            else if (currentline.startsWith("*heat", Qt::CaseInsensitive))
            {
                readHeat(mat);
            }
            else if (currentline.startsWith("*creep", Qt::CaseInsensitive))
            {
                readCreep(mat);
            }
            else if (currentline.startsWith("*cyclichardening", Qt::CaseInsensitive))
            {
                readCyclicHardeningWithTable(mat);
            }
            else if (currentline.startsWith("*elastic", Qt::CaseInsensitive))
            {
                readFlexMat(mat);
            }
            else if (currentline.startsWith("*ratedependent", Qt::CaseInsensitive))
            {
                readRateDependentMat(mat);
            }
            else if (currentline.startsWith("*plastic", Qt::CaseInsensitive))
            {
                QString number = getKeyValue(currentline, "number", "=");
                QString hardening = getKeyValue(currentline, "hardening", "=");
                if (!number.isEmpty())
                {
                    readPlasticityWithOrderMat(mat);
                }
                else
                {
                    if(hardening.isEmpty())
                        readPlasticityJohnsonMat(mat);
                    else
                        readPlasticityComboMat(mat);
                }
            }
            else if (currentline.startsWith("*GasketThicknessBehavior", Qt::CaseInsensitive))
            {
                readThicknessBehaviorMat(mat);
            }
            else if (currentline.startsWith("*GasketElasticity", Qt::CaseInsensitive))
            {
                QString component = getKeyValue(currentline, "component", "=");
                if (!component.compare("membrane", Qt::CaseInsensitive))
                {
                    readFilmBehaviorMat(mat);
                }
                else if (!component.compare("stresstiffness", Qt::CaseInsensitive))
                {
                    readCutBehaviorMat(mat);
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
            else if (currentline.startsWith("*failstrain", Qt::CaseInsensitive))
            {
                readFailureEffect(mat);
            }
            else if (currentline.startsWith("*failstress", Qt::CaseInsensitive))
            {
                readFailureStress(mat);
            }
            else if (currentline.startsWith("*energydamage", Qt::CaseInsensitive))
            {
                ReadEnergyDamager(mat);
            }
            else if (currentline.startsWith("*hashindamage", Qt::CaseInsensitive))
            {
                ReadHashIndamage(mat);
            }
            else if (currentline.startsWith("*phaseparameter", Qt::CaseInsensitive))
            {
                readPhaseParam(mat);
            }
            else if (currentline.startsWith("*sheardamage", Qt::CaseInsensitive))
            {
                readShearDamage(mat);
            }
            else if (currentline.startsWith("*DamageEvolution", Qt::CaseInsensitive))
            {
                readDamageEvolution(mat);
            }
            else
            {
                break;
            }
        }
    }

    void mIMxFile::ReadHashIndamage(shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString type = getKeyValue(currentline, "type", "=").trimmed();
        QString alpha = getKeyValue(currentline, "alpha", "=").trimmed();
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");

        appendMatPropertyValue(mat, MAT_FailureHashin, MatProp_Alpha,
            alpha
        );
        appendMatPropertyValue(mat, MAT_FailureHashin, MatProp_Xt,
            getQStringListIndexValue(_linelist, 0));

        appendMatPropertyValue(mat, MAT_FailureHashin, MatProp_Xc,
            getQStringListIndexValue(_linelist, 1));

        appendMatPropertyValue(mat, MAT_FailureHashin, MatProp_Yt,
            getQStringListIndexValue(_linelist, 2));

        appendMatPropertyValue(mat, MAT_FailureHashin, MatProp_Yc,
            getQStringListIndexValue(_linelist, 3));

        appendMatPropertyValue(mat, MAT_FailureHashin, MatProp_Sl,
            getQStringListIndexValue(_linelist, 4));

        appendMatPropertyValue(mat, MAT_FailureHashin, MatProp_St,
            getQStringListIndexValue(_linelist, 5));
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::ReadEnergyDamager(shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString type = getKeyValue(currentline, "type", "=").trimmed();
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        if (_linelist.size() >= 4)
        {
            appendMatPropertyValue(mat, MAT_EnergyDamage, MatProp_Gft, _linelist[0]);
            appendMatPropertyValue(mat, MAT_EnergyDamage, MatProp_Gfc, _linelist[1]);
            appendMatPropertyValue(mat, MAT_EnergyDamage, MatProp_Gmt, _linelist[2]);
            appendMatPropertyValue(mat, MAT_EnergyDamage, MatProp_Gmc, _linelist[3]);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("材料%1的渐进损伤参数不符合规范").arg(type));
        }
        currentline = _file.readLine().simplified();

    }



    void mIMxFile::ReadPropertyData()
    {
        QString type = getKeyValue(currentline, "type", "=");
        QString propertyname = getKeyValue(currentline, "name", "=");
        QString materialname = getKeyValue(currentline, "mat", "=");
        QString coordinatename = getKeyValue(currentline, "sys", "=");

        if (!type.compare("structural", Qt::CaseInsensitive))
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*bearingsection", Qt::CaseInsensitive))
            {
                readBearingSection(propertyname);
                return;
            }
            else if (currentline.startsWith("*connectorsection", Qt::CaseInsensitive))
            {
                readConnectingDeputy(propertyname, coordinatename);
                return;
            }

            propertyname = ExistRenamePhysicalName(Physical_Property, propertyname);
            int ID = _modeldata->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            if (currentline.startsWith("*solidsection", Qt::CaseInsensitive))
            {
                MechPropertyData::mSolidHomo* property = new MechPropertyData::mSolidHomo(ID, SolidProperty, propertyname);
                _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Coordinate, coordinatename);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*rodsection", Qt::CaseInsensitive))
            {
                readTorsionBar(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Coordinate, coordinatename);
            }
            else if (currentline.startsWith("*shearplatesection", Qt::CaseInsensitive))
            {
                readShearplatesection(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Coordinate, coordinatename);
            }
            else if (currentline.startsWith("*tubesection", Qt::CaseInsensitive))
            {
                readTubesection(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Coordinate, coordinatename);
            }
            else if (currentline.startsWith("*shellsection", Qt::CaseInsensitive))
            {
                readShellsection(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Coordinate, coordinatename);
            }
            else if (currentline.startsWith("*pbeamsection", Qt::CaseInsensitive) || currentline.startsWith("*beamsection", Qt::CaseInsensitive))
            {
                readBeamsection(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Coordinate, coordinatename);
            }
            else if (currentline.startsWith("*springsection", Qt::CaseInsensitive))
            {
                readSpringsection(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Coordinate, coordinatename);
            }
            else if (currentline.startsWith("*viscousdampsection", Qt::CaseInsensitive))
            {
                readViscousdampsection(ID, propertyname);
            }
            else if (currentline.startsWith("*bushsection", Qt::CaseInsensitive))
            {
                readBushsection(ID, propertyname);
            }
            else if (currentline.startsWith("*planestrainsection", Qt::CaseInsensitive))
            {
                readPlanestrainsection(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
            }
            else if (currentline.startsWith("*planestresssection", Qt::CaseInsensitive))
            {
                readPlanestresssection(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
            }
            else if (currentline.startsWith("*gasketsection", Qt::CaseInsensitive))
            {
                readGasketsection(ID, propertyname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Material, materialname);
                appendMapPhysicalStruct(Physical_Property, ID, Physical_Coordinate, coordinatename);
            }
            else if (currentline.startsWith("*gapunisection", Qt::CaseInsensitive))
            {
                readGapunisection(ID, propertyname);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("截面属性 %1  类型不支持！").arg(propertyname));
                currentline = _file.readLine().simplified();
            }
        }
        else if (!type.compare("layered", Qt::CaseInsensitive))
        {
            propertyname = ExistRenamePhysicalName(Physical_Property, propertyname);
            int ID = _modeldata->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            readPlyLaminate(ID, propertyname);
        }
        else if (!type.compare("multiscale", Qt::CaseInsensitive))
        {
            currentline = _file.readLine().simplified();
            propertyname = ExistRenamePhysicalName(Physical_Property, propertyname);
            int ID = _modeldata->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            readMultiscalesection(ID, propertyname);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("截面属性 %1  类型不支持！").arg(propertyname));
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::ReadSetData()
    {
        QString setname = getKeyValue(currentline, "name", "=");
        setname = ExistRenamePhysicalName(Physical_Set, setname);
        QString settype = getKeyValue(currentline, "type", "=");
        QString generate = getKeyValue(currentline, "generate", "=");
        bool internal = currentline.contains("internal", Qt::CaseInsensitive);

        vector<RenderPickResultData> ids;
        int setID = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        if (!settype.compare("node", Qt::CaseInsensitive))
        {
            MechSetData::mNode* set = new MechSetData::mNode(setID, Node, setname, internal, true);
            _modeldata->addPhysicalField<MechSetData::mSetBase>(set);
            readSetData(ids, generate, _lastMaxNodeID);
            set->setNodeData(ids);
            //登记节点集合名（含内部集合）：点面接触读取时据此回填从面区域标记
            _nodeSetNames.insert(setname);
        }
        else if (!settype.compare("nodesurface", Qt::CaseInsensitive))
        {
            MechSetData::mNodeFace* set = new MechSetData::mNodeFace(setID, NodeSurface, setname, internal, true);
            _modeldata->addPhysicalField<MechSetData::mSetBase>(set);
            readSetData(ids, generate, _lastMaxNodeID, false, setID);
            set->setNodeData(ids);
            //节点面集同为节点区域：登记集合名供点面接触从面回填 slave_region（节点表面模式）
            _nodeSetNames.insert(setname);
        }
        else if (!settype.compare("erppnl", Qt::CaseInsensitive))
        {
            MechSetData::mPanel* set = new MechSetData::mPanel(setID, Panel, setname, internal, true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);

            QList<QString> eleNames;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty()) continue;

                QStringList tokens = currentline.split(",");
                for (const QString& token : tokens)
                {
                    QString elementName = token.trimmed();
                    if (elementName.startsWith('"') && elementName.endsWith('"'))
                        elementName = elementName.mid(1, elementName.length() - 2);
                    elementName = elementName.trimmed();
                    if (elementName.isEmpty()) continue;

                    eleNames.append(elementName);
                    _panelToElements[setID].append(elementName);
                }
            }
            set->setPanelCollectionType(0);
            set->setElenames(eleNames);
        }
        else if (!settype.compare("panel", Qt::CaseInsensitive))
        {
            MechSetData::mPanel* set = new MechSetData::mPanel(setID, Panel, setname, internal, true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);

            QList<QString> eleNames;
            QList<QString> allNames;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty()) continue;

                QStringList tokens = currentline.split(",");
                for (const QString& token : tokens)
                {
                    QString elementName = token.trimmed();
                    if (elementName.startsWith('"') && elementName.endsWith('"'))
                        elementName = elementName.mid(1, elementName.length() - 2);
                    elementName = elementName.trimmed();
                    if (elementName.isEmpty()) continue;

                    _panelToElements[setID].append(elementName);
                    allNames.append(elementName);

                    MechSetData::mSetBase* refSet = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(elementName);
                    if (refSet != nullptr && refSet->getSetType() == Element)
                    {
                        eleNames.append(elementName);
                    }
                }
            }
            set->setPanelCollectionType(1);
            // set->setElenames(eleNames);
            set->setNodeNames(allNames);
        }

        else if (!settype.compare("element", Qt::CaseInsensitive))
        {
            MechSetData::mElementSet* set = new MechSetData::mElementSet(setID, Element, setname, internal, true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
            readSetData(ids, generate, _lastMaxMeshID, true);
            set->setElementSetData(ids);
        }
        else if (!settype.compare("elementsurface", Qt::CaseInsensitive))
        {
            MechSetData::mElementSurface* set = new MechSetData::mElementSurface(setID, ElementSurface, setname, internal, true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
            readSurface(ids, generate, setID);
            set->setElementSurfaceData(ids);
        }
        else if (!settype.compare("elementedge", Qt::CaseInsensitive))
        {
            MechSetData::mElementSurface* set = new MechSetData::mElementSurface(setID, ElementEdge, setname, internal, true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
            readEdge(ids, generate, setID);
            set->setElementSurfaceData(ids);
        }
        else if (!settype.compare("frequency", Qt::CaseInsensitive))
        {
            MechSetData::mFrequencyPoint* set = new MechSetData::mFrequencyPoint(setID, FrequencyPoint, setname, internal, true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);

            int x; double y;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                for (int i = 0; i < _linelist.size(); i++)
                {
                    x = i;
                    y = getQStringListIndexValue(_linelist, i).toDouble();
                    set->addOrder(x, y);
                }
            }
        }
        else if (!settype.compare("nodec", Qt::CaseInsensitive))
        {
            MechSetData::mDOFNode* set = new MechSetData::mDOFNode(setID, DOFNode, setname, internal, true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);

            //readDofSetData(ids, generate, _lastMaxNodeID);

            QString dofstr = "";
            int partID = -1;
            int phyID;

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                dofstr = getQStringListIndexValue(_linelist, 1).trimmed();
                QString id = getQStringListIndexValue(_linelist, 0).trimmed();
                if (id.isEmpty())
                    continue;
                phyID = id.simplified().toInt() + _lastMaxNodeID;
                if (_elementIDToPartID.contains(phyID))
                    partID = _elementIDToPartID[phyID];
                appendRenderPickResultData(ids, phyID, partID);

            }
            set->setNodeData(ids);
            int  idof = 0;
            if (dofstr == "T1")
            {
                idof = static_cast<int>(Dof::T1);
            }
            else if (dofstr == "T2")
            {
                idof = static_cast<int>(Dof::T2);
            }
            else if (dofstr == "T3")
            {
                idof = static_cast<int>(Dof::T3);
            }
            else if (dofstr == "R1")
            {
                idof = static_cast<int>(Dof::R1);
            }
            else if (dofstr == "R2")
            {
                idof = static_cast<int>(Dof::R2);
            }
            else if (dofstr == "R3")
            {
                idof = static_cast<int>(Dof::R3);
            }
            set->setDof(idof);
        }
        else if (!settype.compare("revolutionsurface", Qt::CaseInsensitive) ||
            !settype.compare("cylindersurface", Qt::CaseInsensitive) ||
            !settype.compare("segmentsurface", Qt::CaseInsensitive))
        {
            MechSetData::mSurfaceAnalysis* set = new MechSetData::mSurfaceAnalysis(setID, SurfaceAnalysis, setname, internal, true);
            _modeldata->addPhysicalField<MechSetData::mSetBase>(set);

            // 设置子类型
            if (!settype.compare("revolutionsurface", Qt::CaseInsensitive))
                set->setSurfaceSubType(MechSetData::RevolutionSurface);
            else if (!settype.compare("cylindersurface", Qt::CaseInsensitive))
                set->setSurfaceSubType(MechSetData::CylinderSurface);
            else
                set->setSurfaceSubType(MechSetData::SegmentSurface);

            int lineStage = 0; // 0:坐标系行 1:START行 2+:轮廓行
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                if (lineStage == 0)
                {
                    // 第一行：坐标系参数
                    if (set->surfaceSubType() == MechSetData::RevolutionSurface)
                    {
                        // 旋转：Xa,Ya,Za,Xb,Yb,Zb
                        set->setXa(getQStringListIndexValue(_linelist, 0).toDouble());
                        set->setYa(getQStringListIndexValue(_linelist, 1).toDouble());
                        set->setZa(getQStringListIndexValue(_linelist, 2).toDouble());
                        set->setXb(getQStringListIndexValue(_linelist, 3).toDouble());
                        set->setYb(getQStringListIndexValue(_linelist, 4).toDouble());
                        set->setZb(getQStringListIndexValue(_linelist, 5).toDouble());
                    }
                    else
                    {
                        // 拉伸/线段：原点 + x正轴 + z负轴（9个值）
                        set->setOx(getQStringListIndexValue(_linelist, 0).toDouble());
                        set->setOy(getQStringListIndexValue(_linelist, 1).toDouble());
                        set->setOz(getQStringListIndexValue(_linelist, 2).toDouble());
                        set->setXx(getQStringListIndexValue(_linelist, 3).toDouble());
                        set->setXy(getQStringListIndexValue(_linelist, 4).toDouble());
                        set->setXz(getQStringListIndexValue(_linelist, 5).toDouble());
                        set->setZx(getQStringListIndexValue(_linelist, 6).toDouble());
                        set->setZy(getQStringListIndexValue(_linelist, 7).toDouble());
                        set->setZz(getQStringListIndexValue(_linelist, 8).toDouble());
                    }
                    lineStage++;
                }
                else if (lineStage == 1)
                {
                    // 第二行：START 起点
                    if (set->surfaceSubType() == MechSetData::RevolutionSurface)
                    {
                        set->setR1(getQStringListIndexValue(_linelist, 1).toDouble());
                        set->setZ1(getQStringListIndexValue(_linelist, 2).toDouble());
                    }
                    else
                    {
                        set->setSx(getQStringListIndexValue(_linelist, 1).toDouble());
                        set->setSy(getQStringListIndexValue(_linelist, 2).toDouble());
                    }
                    lineStage++;
                }
                else
                {
                    // 第三行及以后：轮廓 LINE / CIRCL
                    QString rowType = getQStringListIndexValue(_linelist, 0).trimmed().toLower();
                    if (set->surfaceSubType() == MechSetData::RevolutionSurface)
                    {
                        // 旋转：只有一行 LINE 终点 r2,z2
                        set->setR2(getQStringListIndexValue(_linelist, 1).toDouble());
                        set->setZ2(getQStringListIndexValue(_linelist, 2).toDouble());
                    }
                    else if (set->surfaceSubType() == MechSetData::CylinderSurface)
                    {
                        // 拉伸：只有一行 LINE 终点 ex,ey
                        set->setEx(getQStringListIndexValue(_linelist, 1).toDouble());
                        set->setEy(getQStringListIndexValue(_linelist, 2).toDouble());
                    }
                    else
                    {
                        // 解析线段：多行 LINE / CIRCL
                        MechSetData::SegmentContourRow row;
                        row.rowType = rowType;
                        row.x1 = getQStringListIndexValue(_linelist, 1).toDouble();
                        row.y1 = getQStringListIndexValue(_linelist, 2).toDouble();
                        if (rowType == "circl")
                        {
                            row.x2 = getQStringListIndexValue(_linelist, 3).toDouble();
                            row.y2 = getQStringListIndexValue(_linelist, 4).toDouble();
                        }
                        set->appendContour(row);
                    }
                }
            }
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("集合 %1  类型不支持！").arg(setname));
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::ReadNonStructQualityData()
    {
        if (!currentline.startsWith("*nsm", Qt::CaseInsensitive))
            return;

        QStringList lines = currentline.split(",");
        QString lineZero = lines.at(0).trimmed();
        QString setname = getKeyValue(currentline, "name", "=");
        QString settype = getKeyValue(currentline, "type", "=").trimmed();
        setname = ExistRenamePhysicalName(Physical_NonStructuralQuality, setname);

        Non typeIndex;
        if (lineZero == "*nsm")
            typeIndex = Non::Union;
        else if (lineZero == "*nsm1")
            typeIndex = Non::MultUnion;
        else if (lineZero == "*nsml")
            typeIndex = Non::ALL;
        else if (lineZero == "*nsml1")
            typeIndex = Non::MultAll;
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("非结构质量 %1  类型不支持！currentline(%2)").arg(setname).arg(currentline));
            currentline = _file.readLine().simplified();
            return;
        }


        PhysicalFieldType propIndex;
        if (!settype.compare("property", Qt::CaseInsensitive))
            propIndex = Physical_Property;
        else if (!settype.compare("element", Qt::CaseInsensitive))
            propIndex = Physical_Element;
        else if (!settype.compare("set", Qt::CaseInsensitive))
            propIndex = Physical_Set;
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("非结构质量 %1  类型不支持！currentline(%2)").arg(setname).arg(currentline));
            currentline = _file.readLine().simplified();
            return;
        }

        int nonId = _modeldata->getManager<MechCaseData::mNonStructQualityManager>(Physical_NonStructuralQuality).createID();
        MechCaseData::mNonStructuralQuality* nonData = new MechCaseData::mNonStructuralQuality(nonId);
        nonData->setNsmName(setname);
        nonData->setTypeIndex(typeIndex);
        nonData->setPropIndex(propIndex);
        _modeldata->getManager<MechCaseData::mNonStructQualityManager>(Physical_NonStructuralQuality)
            .AddNsmData(nonData);
        _modeldata->addPhysicalField<MechCaseData::mNonStructuralQuality>(nonData);

        bool isMult = (typeIndex == Non::MultUnion || typeIndex == Non::MultAll);
        bool isProperty = (propIndex == Physical_Property);
        bool isElement = (propIndex == Physical_Element);
        bool isSet = (propIndex == Physical_Set);

        if (isMult)
        {
            currentline = _file.readLine().simplified();
            nonData->setUnionValue(currentline);
        }

        std::map<QString, QString> dataMap;
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*"))
                break;
            if (currentline.isEmpty())
                continue;

            _linelist = currentline.split(",");
            if (isMult)
            {
                for (int i = 0; i < _linelist.size(); ++i)
                {
                    QString val = getQStringListIndexValue(_linelist, i).trimmed();
                    if (isProperty || isSet)
                    {
                        if (val.startsWith('"') && val.endsWith('"'))
                            val = val.mid(1, val.length() - 2);
                    }
                    if (!val.isEmpty())
                    {
                        dataMap[val] = "";
                        if (isProperty)
                            _nonToPropertyName[nonId].append(val);
                        else if (isElement)
                            _nonToElementId[nonId].append(val.toInt());
                        else if (isSet)
                            _nonToSetName[nonId].append(val);
                    }
                }
            }
            else
            {
                for (int i = 0; i + 1 < _linelist.size(); i += 2)
                {
                    QString key = getQStringListIndexValue(_linelist, i).trimmed();
                    QString val = getQStringListIndexValue(_linelist, i + 1).trimmed();
                    if (isProperty || isSet)
                    {
                        if (key.startsWith('"') && key.endsWith('"'))
                            key = key.mid(1, key.length() - 2);
                    }
                    if (!key.isEmpty())
                    {
                        dataMap[key] = val;
                        if (isProperty)
                            _nonToPropertyName[nonId].append(key);
                        else if (isElement)
                            _nonToElementId[nonId].append(key.toInt());
                        else if (isSet)
                            _nonToSetName[nonId].append(key);
                    }
                }
            }
        }

        if (isProperty)
            nonData->setCrossPropertyData(dataMap);
        else if (isElement)
            nonData->setElementData(dataMap);
        else if (isSet)
            nonData->setElementSetData(dataMap);
    }

    QStringList mIMxFile::getXOY(const QVector3D& xPoint, const QVector3D& origin, const QVector3D& xyPoint) const
    {
        QVector3D x(1, 0, 0), y(0, 1, 0), z(0, 0, 1);
        x = xPoint - origin;
        y = xyPoint - origin;
        // 归一化
        if (!x.isNull()) x.normalize();
        if (!y.isNull()) y.normalize();
        if (!z.isNull()) z.normalize();

        //根据xy平面计算z
        z = QVector3D::crossProduct(x, y);
        z.normalize();

        QString axisLine = QString("%1, %2, %3").arg(origin.x()).arg(origin.y()).arg(origin.z());

        QString zLine = QString(" %1, %2, %3").arg(origin.x() + z.x()).arg(origin.y() + z.y()).arg(origin.z() + z.z());

        QString xLine = QString(" %1, %2, %3").arg(origin.x() + x.x()).arg(origin.y() + x.y()).arg(origin.z() + x.z());

        return { axisLine, zLine, xLine, };
    }

    void mIMxFile::readDiscreteField()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_Field, name);
        QString type = getKeyValue(currentline, "type", "=").trimmed();
        QString location = getKeyValue(currentline, "location", "=").trimmed();
        //导入的坐标默认按Zox转换
        QString ttype = getKeyValue(currentline, "ttype", "=").trimmed();
        
        MxFunctions::PickDataType locateType = MxFunctions::PickDataType::None;
        if (!location.compare("node", Qt::CaseInsensitive))
            locateType = MxFunctions::PickDataType::Node;
        else if (!location.compare("element", Qt::CaseInsensitive))
            locateType = MxFunctions::PickDataType::Element;

        auto skipToNextBlock = [&]()
            {
                while (!_file.atEnd())
                {
                    currentline = _file.readLine().simplified();
                    if (currentline.startsWith("*"))
                        break;
                }
            };

        // 先解析 type，确定 spaceType 和 coorType，并做 Node 过滤
        MxFunctions::DiscreteSpace spaceType = MxFunctions::DiscreteSpace::AngelSpace;
        MxFunctions::CoordinateType coorType = MxFunctions::CoordinateType::Cartesian;
        bool validType = true;

        if (!type.compare("angle", Qt::CaseInsensitive))
        {
            if (locateType == MxFunctions::PickDataType::Node)
            {
                mGlobalSignals::getInstance()->outputMessageSig(1,
                    QString("离散场 %1 节点模式不支持角度场，已跳过").arg(name));
                validType = false;
            }
            spaceType = MxFunctions::DiscreteSpace::AngelSpace;
        }
        else if (!type.compare("thick", Qt::CaseInsensitive))
        {
            spaceType = MxFunctions::DiscreteSpace::ThicknessSpace;
        }
        else if (!type.compare("rectangular", Qt::CaseInsensitive)
            || !type.compare("cylindrical", Qt::CaseInsensitive)
            || !type.compare("spherical", Qt::CaseInsensitive))
        {
            if (locateType == MxFunctions::PickDataType::Node)
            {
                mGlobalSignals::getInstance()->outputMessageSig(1,
                    QString("离散场 %1 节点模式不支持坐标系场，已跳过").arg(name));
                validType = false;
            }
            spaceType = MxFunctions::DiscreteSpace::CoorSpace;
            if (!type.compare("rectangular", Qt::CaseInsensitive))
                coorType = MxFunctions::CoordinateType::Cartesian;
            else if (!type.compare("cylindrical", Qt::CaseInsensitive))
                coorType = MxFunctions::CoordinateType::Cylindrical;
            else if (!type.compare("spherical", Qt::CaseInsensitive))
                coorType = MxFunctions::CoordinateType::Spherical;
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("离散场 %1 类型不支持！").arg(name));
            validType = false;
        }

        if (!validType)
        {
            skipToNextBlock();
            return;
        }

        // 确认有效后再创建对象，避免无效数据残留
        int discreteId = _modeldata->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field).createID();
        MechDiscreteFieldData::mDiscreteField* discreteData = new MechDiscreteFieldData::mDiscreteField(discreteId, name);
        _modeldata->addPhysicalField<MechDiscreteFieldData::mDiscreteField>(discreteData);
        _currentDiscreteField = discreteData;

        discreteData->setLocate(locateType);
        discreteData->setSpaceType(spaceType);
        if (spaceType == MxFunctions::DiscreteSpace::CoorSpace)
            discreteData->setCoorType(coorType);

        // 按 spaceType 统一读取数据
        if (spaceType == MxFunctions::DiscreteSpace::CoorSpace)
        {
            if (locateType == MxFunctions::PickDataType::Node)
            {
                skipToNextBlock();
                return;
            }

            std::map<int, QStringList> elementMap;
            // 默认行（key 为 0）在数据行之前，单独读取一次
            if (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (!currentline.startsWith("*") && !currentline.isEmpty())
                {
                    _linelist = currentline.split(",");
                    if (getQStringListIndexValue(_linelist, 0).trimmed() == "0" && _linelist.size() >= 10)
                    {
                        QVector3D p1(getQStringListIndexValue(_linelist, 1).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 2).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 3).trimmed().toDouble());
                        QVector3D p2(getQStringListIndexValue(_linelist, 4).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 5).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 6).trimmed().toDouble());
                        QVector3D p3(getQStringListIndexValue(_linelist, 7).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 8).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 9).trimmed().toDouble());
                        if (ttype == "xoy")
                        {
                            QStringList coorValues = getXOY(p2, p1, p3);
                            p1 = QVector3D(coorValues[0].split(",")[0].trimmed().toDouble(),
                                coorValues[0].split(",")[1].trimmed().toDouble(),
                                coorValues[0].split(",")[2].trimmed().toDouble());
                            p2 = QVector3D(coorValues[1].split(",")[0].trimmed().toDouble(),
                                coorValues[1].split(",")[1].trimmed().toDouble(),
                                coorValues[1].split(",")[2].trimmed().toDouble());
                            p3 = QVector3D(coorValues[2].split(",")[0].trimmed().toDouble(),
                                coorValues[2].split(",")[1].trimmed().toDouble(),
                                coorValues[2].split(",")[2].trimmed().toDouble());
                        }
                        discreteData->setDirectPoint1(p1);
                        discreteData->setDirectPoint2(p2);
                        discreteData->setDirectPoint3(p3);
                    }
                }
            }

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*")) break;
                if (currentline.isEmpty()) continue;
                _linelist = currentline.split(",");
                if (_linelist.size() >= 10)
                {
                    bool ok = false;
                    int key = getQStringListIndexValue(_linelist, 0).trimmed().toInt(&ok);
                    if (!ok) continue;
                    QStringList values;
                    QStringList valuesZox;
                    // 按个数顺序：一个(key)，三个、三个、三个...
                    QVector3D origin, xP, xyP;
                    int count = 0;
                    for (int i = 1; i + 2 < _linelist.size(); i += 3)
                    {
                        QString v1 = getQStringListIndexValue(_linelist, i).trimmed();
                        QString v2 = getQStringListIndexValue(_linelist, i + 1).trimmed();
                        QString v3 = getQStringListIndexValue(_linelist, i + 2).trimmed();

                        valuesZox.append(QString("%1,%2,%3").arg(v1).arg(v2).arg(v3));

                        if (count == 0)
                        {
                            origin = QVector3D(v1.toDouble(), v2.toDouble(), v3.toDouble());
                        }
                        else if (count == 1)
                        {
                            xP = QVector3D(v1.toDouble(), v2.toDouble(), v3.toDouble());
                        }
                        else if (count == 2)
                        {
                            xyP = QVector3D(v1.toDouble(), v2.toDouble(), v3.toDouble());
                        }

                        count++;
                    }
                    if (ttype == "xoy")
                        values = getXOY(xP, origin, xyP);
                    else
                    {
                        values = valuesZox;
                    }
                    elementMap[key] = values;
                }
            }
            discreteData->setElementCoorMap(elementMap);
        }
        else
        {
            std::map<int, QString> map;
            // 默认行（key 为 0）通常在数据之前，单独处理一次
            if (!_file.atEnd() && locateType == MxFunctions::PickDataType::Element)
            {
                currentline = _file.readLine().simplified();
                if (!currentline.startsWith("*") && !currentline.isEmpty())
                {
                    _linelist = currentline.split(",");
                    QString defVal = getQStringListIndexValue(_linelist, 1).trimmed();
                    if (spaceType == MxFunctions::DiscreteSpace::AngelSpace)
                        discreteData->setDefaultAngel(defVal);
                    else if (spaceType == MxFunctions::DiscreteSpace::ThicknessSpace)
                        discreteData->setDefaultThickness(defVal);
                }
            }

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*")) break;
                if (currentline.isEmpty()) continue;
                _linelist = currentline.split(",");
                if (_linelist.size() < 2) continue;

                bool ok = false;
                int key = getQStringListIndexValue(_linelist, 0).trimmed().toInt(&ok);
                if (!ok) continue;
                map[key] = getQStringListIndexValue(_linelist, 1).trimmed();
            }

            if (spaceType == MxFunctions::DiscreteSpace::AngelSpace)
            {
                if (locateType == MxFunctions::PickDataType::Element)
                    discreteData->setElementAngelMap(map);
            }
            else if (spaceType == MxFunctions::DiscreteSpace::ThicknessSpace)
            {
                if (locateType == MxFunctions::PickDataType::Element)
                    discreteData->setElementThicknessMap(map);
                else
                    discreteData->setNodeThicknessMap(map);
            }
        }

        _currentDiscreteField = nullptr;
    }

    void mIMxFile::readAnalysisField()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_Field, name);
        QString location = getKeyValue(currentline, "location", "=").trimmed();
        QString componentsStr = getKeyValue(currentline, "components", "=").trimmed();
        QString method = getKeyValue(currentline, "method", "=").trimmed();
        QString type = getKeyValue(currentline, "type", "=").trimmed();
        QString csysName = getKeyValue(currentline, "csys", "=");
        QString targetName = getKeyValue(currentline, "target", "=");
        QString filePath = getKeyValue(currentline, "file", "=");
        QString startRowStr = getKeyValue(currentline, "startrow", "=").trimmed();
        QString endRowStr = getKeyValue(currentline, "endrow", "=").trimmed();
        QString startColStr = getKeyValue(currentline, "startcol", "=").trimmed();
        QString defaultValue = getKeyValue(currentline, "default", "=");
        QString scaleModeStr = getKeyValue(currentline, "scalemode", "=").trimmed();
        QString scaleStr = getKeyValue(currentline, "scale", "=").trimmed();
        QString scaleXStr = getKeyValue(currentline, "scalex", "=").trimmed();
        QString scaleYStr = getKeyValue(currentline, "scaley", "=").trimmed();
        QString scaleZStr = getKeyValue(currentline, "scalez", "=").trimmed();
        QString tolTypeStr = getKeyValue(currentline, "tolerancetype", "=").trimmed();
        QString pTolStr = getKeyValue(currentline, "ptol", "=").trimmed();
        QString nTolStr = getKeyValue(currentline, "ntol", "=").trimmed();
        QString nbrTolStr = getKeyValue(currentline, "nbrtol", "=").trimmed();
        QString iTolStr = getKeyValue(currentline, "itol", "=").trimmed();
        QString algStr = getKeyValue(currentline, "algorithm", "=").trimmed();

        MxFunctions::PickDataType locateType = MxFunctions::PickDataType::Node;
        if (!location.compare("element", Qt::CaseInsensitive))
            locateType = MxFunctions::PickDataType::Element;
        else if (!location.compare("elementface", Qt::CaseInsensitive))
            locateType = MxFunctions::PickDataType::ElementFace;

        int fieldID = _modeldata->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field).createID();
        MechDiscreteFieldData::mAnalysisField* fieldData = new MechDiscreteFieldData::mAnalysisField(fieldID, name);
        _modeldata->addPhysicalField<MechDiscreteFieldData::mAnalysisField>(fieldData);

        fieldData->setLocate(locateType);
        fieldData->setComponentCount(qMax(1, componentsStr.toInt()));
        fieldData->setDefineMethod(!method.compare("file", Qt::CaseInsensitive)
            ? MechDiscreteFieldData::mAnalysisField::DefineMethod::TextFile
            : MechDiscreteFieldData::mAnalysisField::DefineMethod::Inline);
        fieldData->setDataType(!type.compare("discrete", Qt::CaseInsensitive)
            ? MechDiscreteFieldData::mAnalysisField::DataType::ObjectDiscrete
            : MechDiscreteFieldData::mAnalysisField::DataType::PointCloud);

        //文本文档定义参数
        if (!filePath.isEmpty())
            fieldData->setTextFilePath(filePath);
        if (!startRowStr.isEmpty())
            fieldData->setTextStartRow(qMax(1, startRowStr.toInt()));
        if (!endRowStr.isEmpty())
            fieldData->setTextEndRow(qMax(1, endRowStr.toInt()));
        if (!startColStr.isEmpty())
            fieldData->setTextStartColumn(qMax(1, startColStr.toInt()));
        fieldData->setDefaultValue(defaultValue);

        //映射空间定义（仅点云）
        if (!scaleModeStr.compare("uniform", Qt::CaseInsensitive))
            fieldData->setScaleMode(MechDiscreteFieldData::mAnalysisField::ScaleMode::Uniform);
        else if (!scaleModeStr.compare("nonuniform", Qt::CaseInsensitive))
            fieldData->setScaleMode(MechDiscreteFieldData::mAnalysisField::ScaleMode::NonUniform);
        if (!scaleStr.isEmpty())
            fieldData->setUniformScale(scaleStr.toDouble());
        if (!scaleXStr.isEmpty())
            fieldData->setScaleX(scaleXStr.toDouble());
        if (!scaleYStr.isEmpty())
            fieldData->setScaleY(scaleYStr.toDouble());
        if (!scaleZStr.isEmpty())
            fieldData->setScaleZ(scaleZStr.toDouble());

        //映射控制（仅点云）
        if (!tolTypeStr.compare("absolute", Qt::CaseInsensitive))
            fieldData->setToleranceType(MechDiscreteFieldData::mAnalysisField::ToleranceType::Absolute);
        if (!pTolStr.isEmpty())
            fieldData->setPositiveTolerance(pTolStr.toDouble());
        if (!nTolStr.isEmpty())
            fieldData->setNegativeTolerance(nTolStr.toDouble());
        if (!nbrTolStr.isEmpty())
            fieldData->setNeighborhoodTolerance(nbrTolStr.toDouble());
        if (!iTolStr.isEmpty())
            fieldData->setInterpolationTolerance(iTolStr.toDouble());
        if (!algStr.compare("volumetric", Qt::CaseInsensitive))
            fieldData->setMappingAlgorithm(MechDiscreteFieldData::mAnalysisField::MappingAlgorithm::Volumetric);

        //坐标系/目标集合映射（csys/target 为名称，缺省即全局坐标系/整个模型）
        //csys/target 写出时带引号，getKeyValue 返回的值含引号，这里去掉（全局坐标系/整个模型不写关键字）
        csysName.remove('\"');
        targetName.remove('\"');
        if (!csysName.isEmpty())
            appendMapPhysicalStruct(Physical_Field, fieldID, Physical_Coordinate, csysName);
        if (!targetName.isEmpty())
            appendMapPhysicalStruct(Physical_Field, fieldID, Physical_Set, targetName);

        //数据行：点云 x,y,z,分量...；节点/单元 id,分量...；单元面 elementid,faceid,分量...
        const bool isPointCloud = fieldData->getDataType() == MechDiscreteFieldData::mAnalysisField::DataType::PointCloud;
        const bool isFace = locateType == MxFunctions::PickDataType::ElementFace;
        const int compCount = fieldData->getComponentCount();
        QList<MechDiscreteFieldData::AnalysisFieldRow> rows;
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*")) break;
            if (currentline.isEmpty()) continue;
            _linelist = currentline.split(",");
            MechDiscreteFieldData::AnalysisFieldRow row;
            int valueStart = 1;
            if (isPointCloud)
            {
                if (_linelist.size() < 3) continue;
                row.x = getQStringListIndexValue(_linelist, 0).trimmed();
                row.y = getQStringListIndexValue(_linelist, 1).trimmed();
                row.z = getQStringListIndexValue(_linelist, 2).trimmed();
                valueStart = 3;
            }
            else if (isFace)
            {
                if (_linelist.size() < 2) continue;
                row.elementId = getQStringListIndexValue(_linelist, 0).trimmed();
                row.faceId = getQStringListIndexValue(_linelist, 1).trimmed();
                valueStart = 2;
            }
            else
            {
                row.id = getQStringListIndexValue(_linelist, 0).trimmed();
                if (row.id.isEmpty()) continue;
            }
            for (int i = 0; i < compCount; ++i)
                row.values.append(getQStringListIndexValue(_linelist, valueStart + i).trimmed());
            rows.append(row);
        }
        fieldData->setRows(rows);
    }

    void mIMxFile::ReadContactData()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_Contact, name);
        QString type = getKeyValue(currentline, "type", "=");
        QString smallsliding = getKeyValue(currentline, "smallsliding", "=");
        QString autosearch = getKeyValue(currentline, "autosearch", "=");
        QString selfcontact = getKeyValue(currentline, "selfcontact", "=");
        QString updatetype = getKeyValue(currentline, "updatetype", "=");

        int phyID = _modeldata->getManager<MechContactData::mContactManager>(Physical_Contact).createID();
        MechContactData::mContactBase* contact = contact = new MechContactData::mContactBase(phyID, SurfaceToSurfaceContact, name, true);
        _modeldata->addPhysicalField<MechContactData::mContactBase>(contact);
        if (!smallsliding.compare("on", Qt::CaseInsensitive))
            contact->smallSlip = true;
        else if (!smallsliding.compare("off", Qt::CaseInsensitive))
            contact->smallSlip = false;

        if (!type.compare("nodetosurface", Qt::CaseInsensitive))
        {
            readNodeToSurfaceContact(contact);
        }
        else if (!type.compare("surfacetosurface", Qt::CaseInsensitive) && !autosearch.compare("on", Qt::CaseInsensitive))
        {
            readCommonContact(contact);
        }
        else if (!type.compare("surfacetosurface", Qt::CaseInsensitive) && currentline.contains("selfcontact", Qt::CaseInsensitive))
        {
            readSelfContact(contact);
        }
        else if (!type.compare("surfacetosurface", Qt::CaseInsensitive))
        {
            readSurfaceToSurfaceContact(contact);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("接触 %1  类型不支持！").arg(name));
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readPeriodicBoundaryData()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_PeriodicBoundary, name);
        QString type = getKeyValue(currentline, "type", "=");
        // 解析adjust参数（独立头部字段）
        QString adjustStr = getKeyValue(currentline, "adjust", "=");
        bool isAdjust = (adjustStr.compare("on", Qt::CaseInsensitive) == 0);

        MxFunctions::PeriodicBoundaryType periodicType = PeriodicBoundary_Translation;
        if (!type.compare("rotation", Qt::CaseInsensitive))
        {
            periodicType = PeriodicBoundary_Rotation;
        }
        else if (!type.compare("translation", Qt::CaseInsensitive))
        {
            periodicType = PeriodicBoundary_Translation;
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("周期性边界 %1 类型不支持！").arg(name));
            return;
        }

        int phyID = _modeldata->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).createID();
        MechContactData::mPeriodicBoundary* periodicData = new MechContactData::mPeriodicBoundary(phyID, periodicType, name, true);
        _modeldata->addPhysicalField<MechContactData::mPeriodicBoundary>(periodicData);

        // 设置调整开关
        periodicData->setAdjust(isAdjust);

        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*") || currentline.isEmpty())
        {
            return;
        }

        _linelist = currentline.split(",");
        // 最少9个参数，少于直接报错
        if (_linelist.size() < 9)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("周期性边界 %1 参数不符合规范！").arg(name));
            return;
        }

        QString slaveSetName = getQStringListIndexValue(_linelist, 0);
        slaveSetName = ExistRenamePhysicalName(Physical_Set, slaveSetName);
        QString masterSetName = getQStringListIndexValue(_linelist, 1);
        masterSetName = ExistRenamePhysicalName(Physical_Set, masterSetName);

        periodicData->startPoint = QVector3D(
            getQStringListIndexValue(_linelist, 2).trimmed().toDouble(),
            getQStringListIndexValue(_linelist, 3).trimmed().toDouble(),
            getQStringListIndexValue(_linelist, 4).trimmed().toDouble());

        periodicData->endPoint = QVector3D(
            getQStringListIndexValue(_linelist, 5).trimmed().toDouble(),
            getQStringListIndexValue(_linelist, 6).trimmed().toDouble(),
            getQStringListIndexValue(_linelist, 7).trimmed().toDouble());

        QString value = getQStringListIndexValue(_linelist, 8).trimmed();
        periodicData->value = value;

        // 根据参数个数自动识别自动/手动模式
        if (_linelist.size() >= 10)
        {
            // 10个参数 → 手动指定模式，读取tol
            QString tolStr = getQStringListIndexValue(_linelist, 9).trimmed();
            bool ok;
            double tol = tolStr.toDouble(&ok);
            // 手动值限制正数，不合法修正为1.0
            if (!ok || tol <= 0)
            {
                tol = 1.0;
            }
            periodicData->setManualValue(tol);
            periodicData->setAutoCalc(false);
        }
        else
        {
            // 9个参数 → 自动计算模式
            periodicData->setAutoCalc(true);
        }

        // 统一使用 DirectInput 模式保存方向控件状态
        periodicData->dirPickState = DirectionPickState();
        periodicData->dirPickState.mode = 6; // mDirectionSelectorWidget::DirectInput
        periodicData->dirPickState.directInputDir = periodicData->endPoint - periodicData->startPoint;

        appendMapPhysicalStruct(Physical_PeriodicBoundary, phyID, Physical_Set, slaveSetName, MapRelation_Slave);
        appendMapPhysicalStruct(Physical_PeriodicBoundary, phyID, Physical_Set, masterSetName, MapRelation_Master);
    }

    void mIMxFile::ReadContactinterference()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_Contact, name);
        QString shrink = getKeyValue(currentline, "shrink", "=");
        QString function = getKeyValue(currentline, "function", "=");

        int phyID = _modeldata->getManager<MechContactData::mContactManager>(Physical_Contact).createID();
        MechContactData::mContactBase* contact = contact = new MechContactData::mContactBase(phyID, InterferenceContact, name, true);
        _modeldata->addPhysicalField<MechContactData::mContactBase>(contact);
        if (!shrink.compare("on", Qt::CaseInsensitive))
            contact->clr_fit_typ = 0;
        else
            contact->clr_fit_typ = 1;

        appendMapPhysicalStruct(Physical_Contact, phyID, Physical_Function, function);

        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        QString slaveSet = getQStringListIndexValue(_linelist, 0);
        slaveSet = ExistRenamePhysicalName(Physical_Set, slaveSet);
        QString masterSet = getQStringListIndexValue(_linelist, 1);
        masterSet = ExistRenamePhysicalName(Physical_Set, masterSet);

        _contactinterference_hash[{ slaveSet, masterSet }] = phyID;

        QString val = getQStringListIndexValue(_linelist, 2);
        if (!val.isEmpty()) contact->size = val;
    }

    void mIMxFile::ReadLoadData()
    {
        if (currentline.startsWith("*nodalload", Qt::CaseInsensitive))
        {
            readNodalload();
        }
        else if (currentline.startsWith("*bodyload", Qt::CaseInsensitive))
        {
            readBodyload();
        }
        else if(currentline.startsWith("*surfaceload", Qt::CaseInsensitive) && currentline.contains("direction", Qt::CaseInsensitive)) //以surfaceload开头 切包含direction 是表面载荷
        {
            readSurface();
        }
        else if (currentline.startsWith("*surfaceload", Qt::CaseInsensitive)) //以surfaceload开头是压强
        {
            readSurfaceload();
        }
        else if (currentline.startsWith("*nodalflux", Qt::CaseInsensitive))
        {
            readNodalflux();
        }
        else if (currentline.startsWith("*tload", Qt::CaseInsensitive))
        {
            readTransient();
        }
        else if (currentline.startsWith("*rload", Qt::CaseInsensitive) || currentline.startsWith("*rload1", Qt::CaseInsensitive))
        {
            // 提取关键字精确匹配，避免 *rload 与 *rload1 前缀重叠误判
            QString keyword = currentline;
            int pos = currentline.indexOf(',');
            if (pos != -1)
                keyword = currentline.left(pos);
            keyword = keyword.simplified();

            if (keyword.compare("*rload1", Qt::CaseInsensitive) == 0)
            {
                readFreqdomain("*rload1");
            }
            else
            {
                readFreqdomain("*rload");
            }
        }
        else if (currentline.startsWith("*dload", Qt::CaseInsensitive))
        {
            readCombined();
        }
        else if (currentline.startsWith("*sload", Qt::CaseInsensitive))
        {
            readAcousticConcForce();
        }
        else if (currentline.startsWith("*acsrce", Qt::CaseInsensitive))
        {
            readAcousticFrequencyDomain();
        }
        else if (currentline.startsWith("*ds", Qt::CaseInsensitive))
        {
            readSpectrum();
        }
        else if (currentline.startsWith("*domega", Qt::CaseInsensitive))
        {
            readRoACC();
        }
        else if (currentline.startsWith("*omega", Qt::CaseInsensitive))
        {
            readRoSpeed();
        }
        else if (currentline.startsWith("*edgeload", Qt::CaseInsensitive))
        {
            readEdgeload();
        }
    }

    void mIMxFile::readIsolinearelastic(shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        mat->setMaterialType(MaterialType::LinearElastic_Isotropic);
        currentline = _file.readLine().simplified();

        _linelist = currentline.split(",");
        if (dependency.isEmpty())//没有温度
        {
            if (_linelist.size() >= 2)
            {
                appendMatPropertyValue(mat, MAT_Isotropy, MatProp_YoungsModulus, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Isotropy, MatProp_Botson, _linelist[1]);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            appendTempCurve(mat, MAT_Isotropy, MatProp_Curve_Y_B, 3, "本构", dependency);
        }

    }

    void mIMxFile::readOrtholinearelastic2d(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        mat->setMaterialType(LinearElastic_Orthotropic);
        mat->addProperty(MAT_Orthotropy, MatProp_Dimensionality, 0);
        currentline = _file.readLine().simplified();

        _linelist = currentline.split(",");
        if (dependency.isEmpty())//没有温度
        {
            if (_linelist.size() >= 6)
            {
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Ex, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Ey, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_NUxy, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Gxy, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Gxz, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Gyz, _linelist[5]);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            appendTempCurve(mat, MAT_Orthotropy, MatProp_Curve_Orthotropy, 7, "本构", dependency);
        }
    }

    void mIMxFile::readOrtholinearelastic3d(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        QString constants = getKeyValue(currentline, "constants", "=");
        mat->setMaterialType(LinearElastic_Orthotropic);
        bool isd = false;
        if (constants == "off")
        {
            mat->addProperty(MAT_Orthotropy, MatProp_Dimensionality, 2);
            isd = true;
        }
        else
        {
            mat->addProperty(MAT_Orthotropy, MatProp_Dimensionality, 1);
            isd = false;
        }

        currentline = _file.readLine().simplified();

        _linelist = currentline.split(",");
        if (dependency.isEmpty())//没有温度
        {
            if (!isd)
            {
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Ex, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Ey, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Ez, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_NUyz, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_NUxz, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_NUxy, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Gyz, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Gxz, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_Gxy, _linelist[8]);
            }
            else if (isd)
            {
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D1111, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D1122, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D2222, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D1133, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D2233, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D3333, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D1212, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D1313, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Orthotropy, MatProp_D2323, _linelist[8]);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            appendTempCurve(mat, MAT_Orthotropy, MatProp_Curve_Orthotropy, 10, "本构", dependency);
        }
    }

    void mIMxFile::readAnisotropiclinearelastic2d(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        mat->setMaterialType(LinearElastic_Anisotropic);
        mat->addProperty(MAT_Anisotropy, MatProp_Dimensionality, 0);
        currentline = _file.readLine().simplified();

        _linelist = currentline.split(",");
        if (dependency.isEmpty())//没有温度
        {
            if (_linelist.size() >= 6)
            {
                appendMatPropertyValue(mat, MAT_Anisotropy, MatProp_G11, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Anisotropy, MatProp_G12, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Anisotropy, MatProp_G13, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Anisotropy, MatProp_G22, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Anisotropy, MatProp_G23, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Anisotropy, MatProp_G33, _linelist[5]);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            appendTempCurve(mat, MAT_Anisotropy, MatProp_Curve_Anisotropy_Gn, 7, "本构", dependency);
        }
    }

    void mIMxFile::readAnisotropiclinearelastic3d(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        mat->setMaterialType(LinearElastic_Anisotropic);
        mat->addProperty(MAT_Anisotropy, MatProp_Dimensionality, 1);
        currentline = _file.readLine().simplified();

        _linelist = currentline.split(",");
        if (dependency.isEmpty())//没有温度
        {
            if (_linelist.size() >= 21)
            {
                for (int i = 0; i < 21; i++)
                {
                    appendMatPropertyValue(mat, MAT_Anisotropy, MaterialPropType(MatProp_G11 + i), _linelist[i]);
                }
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            appendTempCurve(mat, MAT_Anisotropy, MatProp_Curve_Anisotropy_Gn, 22, "本构", dependency);
        }
    }

    void mIMxFile::readBreakMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(OtherMaterial_Break);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        if (_linelist.size() >= 4)
        {
            appendMatPropertyValue(mat, MAT_BreakParam, MatProp_YoungsModulus, _linelist[0]);
            appendMatPropertyValue(mat, MAT_BreakParam, MatProp_Botson, _linelist[1]);
            appendMatPropertyValue(mat, MAT_BreakParam, MatProp_Strech_Limit_Strenth, _linelist[2]);
            appendMatPropertyValue(mat, MAT_BreakParam, MatProp_Break_Power, _linelist[3]);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
        }

        readPhaseParam(mat);
    }

    void mIMxFile::readPhaseParam(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*phaseparameter", Qt::CaseInsensitive))
        {
            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            if (_linelist.size() >= 3)
            {
                appendMatPropertyValue(mat, MAT_PhaseFieldParam, MatProp_PhiseFild_Model, getQStringListIndexValue(_linelist, 0), MechCommon::QVariantType::QVariant_Int);
                appendMatPropertyValue(mat, MAT_PhaseFieldParam, MatProp_Cohension_Model, getQStringListIndexValue(_linelist, 1), MechCommon::QVariantType::QVariant_Int);
                appendMatPropertyValue(mat, MAT_PhaseFieldParam, MatProp_Specifile_W, getQStringListIndexValue(_linelist, 2));
            }


        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  缺少相场参数！").arg(mat->getMaterialName()));
        }
    }

    void mIMxFile::readHeatMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        mat->setMaterialType(Thermal_Base);

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*conductivity", Qt::CaseInsensitive))
            {
                QString type = getKeyValue(currentline, "type", "=");
                QString dependency = getKeyValue(currentline, "dependency", "=");

                int size = 1;
                currentline = _file.readLine().simplified();

                if (!type.compare("ortho", Qt::CaseInsensitive))
                {
                    mat->addProperty(MAT_HeatConductivityCoefficient_UnCheck, MatProp_HeatConductivityCoefficient, 1);
                    size = 3;
                }
                else if (!type.compare("aniso", Qt::CaseInsensitive))
                {
                    mat->addProperty(MAT_HeatConductivityCoefficient_UnCheck, MatProp_HeatConductivityCoefficient, 2);
                    size = 6;
                }
                else
                {
                    mat->addProperty(MAT_HeatConductivityCoefficient_UnCheck, MatProp_HeatConductivityCoefficient, 0);
                    size = 1;
                }

                if (dependency.isEmpty())
                {
                    _linelist = currentline.split(",");
                    if (_linelist.size() < size)
                    {
                        mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1 热传导系数参数不符合规范！").arg(mat->getMaterialName()));
                    }
                    for (int i = 0; i < size; i++)
                    {
                        appendMatPropertyValue(mat, MAT_HeatConductivityCoefficient_UnCheck, MaterialPropType(i + MatProp_HeatConductivityCoefficient_A1), _linelist[i]);
                    }
                    currentline = _file.readLine().simplified();
                }
                else
                {
                    appendTempCurve(mat, MAT_HeatConductivityCoefficient_UnCheck, MatProp_Curve_HeatConductivityCoefficient, size + 1, "热传导系数", dependency);
                }

            }
            else if (currentline.startsWith("*specificheat", Qt::CaseInsensitive))
            {
                QString dependency = getKeyValue(currentline, "dependency", "=");
                currentline = _file.readLine().simplified();
                if (dependency.isEmpty())
                {
                    appendMatPropertyValue(mat, MAT_Heat, MatProp_HeatCapacityDensity, currentline);
                    currentline = _file.readLine().simplified();
                }
                else
                {
                    appendTempCurve(mat, MAT_Heat, MatProp_Curve_HeatCapacityDensity, 2, "热容系数", dependency);
                }
            }
            else if (currentline.startsWith("*heatgeneration", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                appendMatPropertyValue(mat, MAT_HeatGenerationRate, MatProp_HeatGenerationRate, currentline);
                currentline = _file.readLine().simplified();
            }
            else
            {
                break;
            }
        }
    }

    void mIMxFile::readChabocheMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(Plastic_Cyclic);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readJohnsonCookMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(Plastic_Johnson);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readShearDamageMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(Damage_Shear);
        currentline = _file.readLine().simplified();    
    }

    void mIMxFile::readShearDamage(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString ks = getKeyValue(currentline, "ks", "=");
        currentline = _file.readLine().simplified();
        appendMatPropertyValue(mat, MAT_ShearDamage, MatProp_KS, ks);

        //无相关曲线直接复用已有逻辑
        appendTempCurve(mat, MAT_ShearDamage, MatProp_Curve_Fs_Ssr_Sr, 3, "断裂曲线", "");

    }

    void mIMxFile::readDamageEvolution(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString rateType = getKeyValue(currentline, "type", "=");
        currentline = _file.readLine().simplified();
        if (!rateType.isEmpty())
        {
            if (!rateType.compare("Displacement", Qt::CaseInsensitive)) {
                mat->addProperty(MAT_DamageEvolution, MatProp_EvolutionType, 0);
                mat->addProperty(MAT_DamageEvolution, MatProp_EvolutionMode, 0);
                appendMatPropertyValue(mat, MAT_DamageEvolution, MatProp_Fd, currentline);
            }
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
        }
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readaCousticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(AcousticMaterial_Base);
        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*density", Qt::CaseInsensitive))
            {
                readDensity(mat);
            }
            else if (currentline.startsWith("*bulkmodulus", Qt::CaseInsensitive))
            {
                readBulkModulus(mat);
            }
            else if (currentline.startsWith("*speed", Qt::CaseInsensitive))
            {
                readSoundVelocity(mat);
            }
            else if (currentline.startsWith("*fluiddamp", Qt::CaseInsensitive))
            {
                readAcousticMaterialDamping(mat);
            }
            else
            {
                break;
            }
        }

    }

    void mIMxFile::readDeformationplasticityMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(Plastic_Damage);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        appendMatPropertyValue(mat, MAT_Deformation, MatProp_YoungsModulus, getQStringListIndexValue(_linelist, 0));
        appendMatPropertyValue(mat, MAT_Deformation, MatProp_Botson, getQStringListIndexValue(_linelist, 1));
        appendMatPropertyValue(mat, MAT_Deformation, MatProp_YieldStress, getQStringListIndexValue(_linelist, 2));
        appendMatPropertyValue(mat, MAT_Deformation, MatProp_StressExponent, getQStringListIndexValue(_linelist, 3));
        appendMatPropertyValue(mat, MAT_Deformation, MatProp_YieldOffset, getQStringListIndexValue(_linelist, 4));
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readViscoplasticityMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(Viscous_Viscoplastic);
        currentline = _file.readLine().simplified();
    }


    void mIMxFile::readViscoelasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(Viscous_Viscoelastic);
        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*elastic", Qt::CaseInsensitive))
            {
                QString moduli = getKeyValue(currentline, "moduli", "=");
                currentline = _file.readLine().simplified();
                if (!moduli.compare("long_term", Qt::CaseInsensitive))
                {
                    mat->addProperty(MAT_Viscoelasticity, MatProp_ModulusTimeScale, 0);
                }
                else if (!moduli.compare("instantaneous", Qt::CaseInsensitive))
                {
                    mat->addProperty(MAT_Viscoelasticity, MatProp_ModulusTimeScale, 1);
                }
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Viscoelasticity, MatProp_YoungsModulus, getQStringListIndexValue(_linelist, 0));
                appendMatPropertyValue(mat, MAT_Viscoelasticity, MatProp_Botson, getQStringListIndexValue(_linelist, 1));
                currentline = _file.readLine().simplified();

            }
            else if (currentline.startsWith("*viscoelastic", Qt::CaseInsensitive))
            {
                QString time = getKeyValue(currentline, "time", "=");
                if (!time.compare("prony", Qt::CaseInsensitive))
                {
                    currentline = _file.readLine().simplified();
                    appendTempCurve(mat, MAT_Viscoelasticity, MatProp_Curve_TimeDomain, 3, "时域Prony曲线", "");
                }

            }
            else if (currentline.startsWith("*trs", Qt::CaseInsensitive))
            {
                QString definition = getKeyValue(currentline, "definition", "=");
                currentline = _file.readLine().simplified();
                if (!definition.compare("wlf", Qt::CaseInsensitive))
                {
                    mat->addProperty(MAT_Viscoelasticity, MatProp_TRSType, 1);
                    _linelist = currentline.split(",");
                    appendMatPropertyValue(mat, MAT_Viscoelasticity, MatProp_Theta0, getQStringListIndexValue(_linelist, 0));
                    appendMatPropertyValue(mat, MAT_Viscoelasticity, MatProp_C1, getQStringListIndexValue(_linelist, 1));
                    appendMatPropertyValue(mat, MAT_Viscoelasticity, MatProp_C2, getQStringListIndexValue(_linelist, 2));
                    currentline = _file.readLine().simplified();
                }
                else if (!definition.compare("arrhenius", Qt::CaseInsensitive))
                {
                    mat->addProperty(MAT_Viscoelasticity, MatProp_TRSType, 2);
                    _linelist = currentline.split(",");
                    appendMatPropertyValue(mat, MAT_Viscoelasticity, MatProp_Theta0, getQStringListIndexValue(_linelist, 0));
                    appendMatPropertyValue(mat, MAT_Viscoelasticity, MatProp_E0, getQStringListIndexValue(_linelist, 1));
                    currentline = _file.readLine().simplified();
                }
            }
            else
            {
                break;
            }
        }
    }

    void mIMxFile::readElastoplasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        //MaterialClass matType = MAT_IsotropicHardening;
        mat->setMaterialType(Plastic_Normal);
        QString temp1;
        QString temp2;
        QVariant curve;
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readShimMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        //MaterialClass matType = MAT_Shim;
        mat->setMaterialType(OtherMaterial_Shim);
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readHyperforMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(OtherMaterial_Foam);
        QString type = getKeyValue(currentline, "N = ");
        if (type.compare("1") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                ReadTestData(mat, MAT_Hyperhorm, MatProp_Curve_Testdatainput);
            }
            else
            {
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu1, _linelist[2]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("2") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                ReadTestData(mat, MAT_Hyperhorm, MatProp_Curve_Testdatainput);
            }
            else
            {
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu1, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu2, _linelist[5]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("3") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                ReadTestData(mat, MAT_Hyperhorm, MatProp_Curve_Testdatainput);
            }
            else
            {
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu3, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha3, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu1, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu2, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu3, _linelist[8]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("4") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                ReadTestData(mat, MAT_Hyperhorm, MatProp_Curve_Testdatainput);
            }
            else
            {
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu3, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha3, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu4, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha4, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu1, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu2, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu3, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu4, _linelist[11]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("5") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                ReadTestData(mat, MAT_Hyperhorm, MatProp_Curve_Testdatainput);
            }
            else
            {
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu3, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha3, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu4, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha4, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu5, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha5, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu1, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu2, _linelist[11]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu3, _linelist[12]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu4, _linelist[13]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu5, _linelist[14]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("6") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                ReadTestData(mat, MAT_Hyperhorm, MatProp_Curve_Testdatainput);
            }
            else
            {
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu3, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha3, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu4, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha4, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu5, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha5, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Mu6, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Alpha6, _linelist[11]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu1, _linelist[12]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu2, _linelist[13]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu3, _linelist[14]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu4, _linelist[15]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu5, _linelist[16]);
                appendMatPropertyValue(mat, MAT_Hyperhorm, MatProp_Nu6, _linelist[17]);
                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::ReadTestData(shared_ptr<MechMaterialData::mMaterialBase> mat, const MaterialClass& matType, const MaterialPropType& propType)
    {

        mat->addProperty(matType, MatProp_ParaDataType, 1);

        QString type = getKeyValue(currentline, "type", "=");
        if (type.compare("uniaxial") == 0)
        {
            currentline = _file.readLine().simplified();
            mat->addProperty(matType, MatProp_TestDataType, 0);
            if (matType == MAT_Hyperhorm)
            {
                appendTempCurve(mat, matType, propType, 3, "单轴拉伸试验", "");
            }
            appendTempCurve(mat, matType, propType, 2, "单轴拉伸试验", "");
        }
        else if (type.compare("volumetric") == 0)
        {
            currentline = _file.readLine().simplified();
            mat->addProperty(matType, MatProp_TestDataType, 1);
            appendTempCurve(mat, matType, propType, 2, "体积试验", "");
        }
    }

    void mIMxFile::readNeoHookean2Mat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(HyperElastic_NeoHookean);
        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
        {
            while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                QString type = getKeyValue(currentline, "type", "=");
                if (type.compare("uniaxial"))
                    ReadTestData(mat, MAT_Neohookean2, MatProp_Curve_Testdatainput_Press);
                if (type.compare("volumetric"))
                    ReadTestData(mat, MAT_Neohookean2, MatProp_Curve_Testdatainput_V);
            }
        }
        else
        {
            mat->addProperty(MAT_Neohookean2, MatProp_ParaDataType, 0);
            _linelist = currentline.split(",");
            appendMatPropertyValue(mat, MAT_Neohookean2, MatProp_C10, _linelist[0]);
            appendMatPropertyValue(mat, MAT_Neohookean2, MatProp_D1, _linelist[1]);
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readMooneyRivlinMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(HyperElastic_MooneyRivlin);
        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
        {
            while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                QString type = getKeyValue(currentline, "type", "=");
                if (type.compare("uniaxial") == 0)
                    ReadTestData(mat, MAT_MooneyRivlin, MatProp_Curve_Testdatainput_Press);
                if (type.compare("volumetric") == 0)
                    ReadTestData(mat, MAT_MooneyRivlin, MatProp_Curve_Testdatainput_V);
            }
        }
        else
        {
            _linelist = currentline.split(",");
            appendMatPropertyValue(mat, MAT_MooneyRivlin, MatProp_C10, _linelist[0]);
            appendMatPropertyValue(mat, MAT_MooneyRivlin, MatProp_C01, _linelist[1]);
            appendMatPropertyValue(mat, MAT_MooneyRivlin, MatProp_D1, _linelist[2]);
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readArrudaBoyceMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {

        mat->setMaterialType(HyperElastic_ArrudaBoyce);
        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
        {
            while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                QString type = getKeyValue(currentline, "type", "=");
                if (type.compare("uniaxial") == 0)
                    ReadTestData(mat, MAT_Arrudaboyce, MatProp_Curve_Testdatainput_Press);
                if (type.compare("volumetric") == 0)
                    ReadTestData(mat, MAT_Arrudaboyce, MatProp_Curve_Testdatainput_V);
            }
        }
        else
        {
            mat->addProperty(MAT_Arrudaboyce, MatProp_ParaDataType, 0);
            _linelist = currentline.split(",");
            appendMatPropertyValue(mat, MAT_Arrudaboyce, MatProp_MU, _linelist[0]);
            appendMatPropertyValue(mat, MAT_Arrudaboyce, MatProp_LambdaM, _linelist[1]);
            appendMatPropertyValue(mat, MAT_Arrudaboyce, MatProp_D, _linelist[2]);
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readYeohMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(HyperElastic_Yeoh);
        currentline = _file.readLine().simplified();

        if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
        {
            while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                QString type = getKeyValue(currentline, "type", "=");
                if (type.compare("uniaxial") == 0)
                    ReadTestData(mat, MAT_Yeoh, MatProp_Curve_Testdatainput_Press);
                if (type.compare("volumetric") == 0)
                    ReadTestData(mat, MAT_Yeoh, MatProp_Curve_Testdatainput_V);
            }
        }
        else
        {
            mat->addProperty(MAT_Yeoh, MatProp_ParaDataType, 0);
            _linelist = currentline.split(",");
            if (_linelist.size() >= 6)
            {
                appendMatPropertyValue(mat, MAT_Yeoh, MatProp_C10, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Yeoh, MatProp_C20, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Yeoh, MatProp_C30, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Yeoh, MatProp_D1, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Yeoh, MatProp_D2, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Yeoh, MatProp_D3, _linelist[5]);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2,
                    QString("材料 %1 的Yeoh参数不符合规范！").arg(mat->getMaterialName()));
            }
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readOgdenMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(HyperElastic_Ogden);
        QString type = getKeyValue(currentline, "N = ");
        appendMatPropertyValue(mat, MAT_Ogden, MatProp_Order, type, MechCommon::QVariantType::QVariant_Int);
        if (type.compare("1") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_Press);
                    if (type.compare("volumetric") == 0)
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_V);
                }
            }
            else
            {
                mat->addProperty(MAT_Ogden, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D1, _linelist[2]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("2") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                //ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Ogden, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D1, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D2, _linelist[5]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("3") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                //ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Ogden, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu3, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha3, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D1, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D2, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D3, _linelist[8]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("4") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                //ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Ogden, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu3, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha3, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu4, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha4, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D1, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D2, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D3, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D4, _linelist[11]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("5") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                //ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Ogden, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu3, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha3, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu4, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha4, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu5, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha5, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D1, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D2, _linelist[11]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D3, _linelist[12]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D4, _linelist[13]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D5, _linelist[14]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("6") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                QString type = getKeyValue(currentline, "type");
                //ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Ogden, MatProp_Curve_Testdatainput_V);
                    }
                }

            }
            else
            {
                mat->addProperty(MAT_Ogden, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu1, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha1, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu2, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha2, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu3, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha3, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu4, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha4, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu5, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha5, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Mu6, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_Alpha6, _linelist[11]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D1, _linelist[12]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D2, _linelist[13]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D3, _linelist[14]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D4, _linelist[15]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D5, _linelist[16]);
                appendMatPropertyValue(mat, MAT_Ogden, MatProp_D6, _linelist[17]);
                currentline = _file.readLine().simplified();
            }
        }

    }

    void mIMxFile::readPolynomialMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mat->setMaterialType(HyperElastic_Polynomial);
        QString type = getKeyValue(currentline, "N = ");
        appendMatPropertyValue(mat, MAT_Polynomial, MatProp_Order, type, MechCommon::QVariantType::QVariant_Int);
        if (type.compare("1") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 1);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C10, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C01, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D1, _linelist[2]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("2") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 1);
                //ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C10, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C01, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C20, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C11, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C02, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D1, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D2, _linelist[6]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("3") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 1);
                //ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C10, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C01, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C20, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C11, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C02, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C30, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C21, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C12, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C03, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D1, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D2, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D3, _linelist[11]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("4") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 1);
                //ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C10, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C01, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C20, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C11, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C02, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C30, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C21, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C12, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C03, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C40, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C31, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C22, _linelist[11]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C13, _linelist[12]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C04, _linelist[13]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D1, _linelist[14]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D2, _linelist[15]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D3, _linelist[16]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D4, _linelist[17]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("5") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 1);
                //ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_V);
                    }
                }
            }
            else
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C10, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C01, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C20, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C11, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C02, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C30, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C21, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C12, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C03, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C40, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C31, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C22, _linelist[11]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C13, _linelist[12]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C04, _linelist[13]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C50, _linelist[14]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C41, _linelist[15]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C32, _linelist[16]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C23, _linelist[17]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C14, _linelist[18]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C05, _linelist[19]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D1, _linelist[20]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D2, _linelist[21]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D3, _linelist[22]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D4, _linelist[23]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D5, _linelist[24]);
                currentline = _file.readLine().simplified();
            }
        }
        if (type.compare("6") == 0)
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 1);
                //ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput);
                while (currentline.startsWith("*testdatainput", Qt::CaseInsensitive))
                {
                    QString type = getKeyValue(currentline, "type", "=");
                    if (type.compare("uniaxial") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_Press);
                    }
                    if (type.compare("volumetric") == 0)
                    {
                        ReadTestData(mat, MAT_Polynomial, MatProp_Curve_Testdatainput_V);
                    }
                }

            }
            else
            {
                mat->addProperty(MAT_Polynomial, MatProp_ParaDataType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C10, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C01, _linelist[1]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C20, _linelist[2]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C11, _linelist[3]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C02, _linelist[4]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C30, _linelist[5]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C21, _linelist[6]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C12, _linelist[7]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C03, _linelist[8]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C40, _linelist[9]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C31, _linelist[10]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C22, _linelist[11]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C13, _linelist[12]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C04, _linelist[13]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C50, _linelist[14]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C41, _linelist[15]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C32, _linelist[16]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C23, _linelist[17]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C14, _linelist[18]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C05, _linelist[19]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C60, _linelist[20]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C51, _linelist[21]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C42, _linelist[22]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C33, _linelist[23]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C24, _linelist[24]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C15, _linelist[25]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_C06, _linelist[26]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D1, _linelist[27]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D2, _linelist[28]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D3, _linelist[29]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D4, _linelist[30]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D5, _linelist[31]);
                appendMatPropertyValue(mat, MAT_Polynomial, MatProp_D6, _linelist[32]);
                currentline = _file.readLine().simplified();
            }
        }
    }

    void mIMxFile::readFlexMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        currentline = _file.readLine().simplified();
        if (dependency.isEmpty())
        {
            _linelist = currentline.split(",");
            if (_linelist.size() >= 2)
            {
                appendMatPropertyValue(mat, MAT_Flex, MatProp_YoungsModulus, _linelist[0]);
                appendMatPropertyValue(mat, MAT_Flex, MatProp_Botson, _linelist[1]);
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            /*if (!dependency.compare("temp", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_Flex, MatProp_Refer_X, 0);
            }
            else if (!dependency.compare("freq", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_Flex, MatProp_Refer_X, 1);
            }*/
            appendTempCurve(mat, MAT_Flex, MatProp_Curve_Y_B, 3, "弹性参数", dependency);
        }
    }

    void mIMxFile::readRateDependentMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString rateType = getKeyValue(currentline, "type", "=");
        currentline = _file.readLine().simplified();
        if (!rateType.isEmpty())
        {
            if (!rateType.compare("JohnsonCook", Qt::CaseInsensitive)) {
                mat->addProperty(MAT_RateDependent, MatProp_RateType, 0);
                _linelist = currentline.split(",");
                appendMatPropertyValue(mat, MAT_RateDependent, MatProp_C, _linelist.value(0, ""));
                appendMatPropertyValue(mat, MAT_RateDependent, MatProp_RefStrainRate, _linelist.value(1, "0"));
            }
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
        }
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readPlasticityWithOrderMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString number = getKeyValue(currentline, "number", "=");
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        int num = number.toInt();
        int expected = num * 2;
        if (_linelist.size() > expected)
        {
            appendMatPropertyValue(mat, MAT_PlasticityWithOrder, MatProp_Order, number, MechCommon::QVariantType::QVariant_Int);
            appendMatPropertyValue(mat, MAT_PlasticityWithOrder, MatProp_InitialYieldStressAtZeroPlasticity, _linelist[0]);
            for (int i = 0; i < expected; i++)
            {
                appendMatPropertyValue(mat, MAT_PlasticityWithOrder, MaterialPropType(MatProp_KinematicHardeningC1 + i), _linelist[i + 1]);
            }
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
        }
        currentline = _file.readLine().simplified();
    }
    
    void mIMxFile::readPlasticityJohnsonMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        appendMatPropertyValue(mat, MAT_PlasticityJohnson, MatProp_A, _linelist.value(0, "0"));
        appendMatPropertyValue(mat, MAT_PlasticityJohnson, MatProp_B, _linelist.value(1, ""));
        appendMatPropertyValue(mat, MAT_PlasticityJohnson, MatProp_N, _linelist.value(2, ""));
        appendMatPropertyValue(mat, MAT_PlasticityJohnson, MatProp_M, _linelist.value(3, ""));
        appendMatPropertyValue(mat, MAT_PlasticityJohnson, MatProp_MeltingTemp, _linelist.value(4, "0"));
        appendMatPropertyValue(mat, MAT_PlasticityJohnson, MatProp_TransitionTemp, _linelist.value(5, "0"));
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readPlasticityComboMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        QString hardening = getKeyValue(currentline, "hardening", "=");

        if (!hardening.compare("kinematic", Qt::CaseInsensitive))
        {
            mat->addProperty(MAT_Plasticity, MatProp_Curve_PlasticityProperties_type, 1);
        }
        else if (!hardening.compare("combined", Qt::CaseInsensitive))
        {
            mat->addProperty(MAT_Plasticity, MatProp_Curve_PlasticityProperties_type, 2);
        }
        else //(!hardening.compare("isotropic", Qt::CaseInsensitive))
        {
            mat->addProperty(MAT_Plasticity, MatProp_Curve_PlasticityProperties_type, 0);
        }
        currentline = _file.readLine().simplified();
        if (dependency.isEmpty())
        {
            appendTempCurve(mat, MAT_Plasticity, MatProp_Curve_PlasticityProperties, 2, "塑性参数", dependency);
        }
        else
        {
            appendTempCurve(mat, MAT_Plasticity, MatProp_Curve_PlasticityProperties, 3, "塑性参数", dependency);
        }
    }

    void mIMxFile::readThicknessBehaviorMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString direction = getKeyValue(currentline, "direction", "=");
        QString type = getKeyValue(currentline, "type", "=");
        if (!type.compare("DAMAGE", Qt::CaseInsensitive))
        {
            mat->addProperty(MAT_ThicknessBehavior, MatProp_ShimThicknessType, 0);
        }
        else if (!type.compare("ELASTIC_PLASTI", Qt::CaseInsensitive))
        {
            mat->addProperty(MAT_ThicknessBehavior, MatProp_ShimThicknessType, 1);
        }

        if (direction.isEmpty())
        {
            currentline = _file.readLine().simplified();
            appendTempCurve(mat, MAT_ThicknessBehavior, MatProp_Curve_ShimLoading, 2, "加载曲线", "");
        }
        else
        {
            currentline = _file.readLine().simplified();
            appendTempCurve(mat, MAT_ThicknessBehavior, MatProp_Curve_ShimUnloading, 3, "卸载曲线", "");
        }
    }

    void mIMxFile::readFilmBehaviorMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        if (_linelist.size() >= 2)
        {
            appendMatPropertyValue(mat, MAT_FilmBehavior, MatProp_YoungsModulus, _linelist[0]);
            appendMatPropertyValue(mat, MAT_FilmBehavior, MatProp_Botson, _linelist[1]);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  参数不符合规范！").arg(mat->getMaterialName()));
        }
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readCutBehaviorMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        appendMatPropertyValue(mat, MAT_CutBehavior, MatProp_ShearStiffness, currentline);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readCyclicHardeningWithTable(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        if (mat->getMaterialType()== Plastic_Cyclic)
        {
            readCyclichardening(mat);
        }
        else
        {
            QString dependency = getKeyValue(currentline, "dependency", "=");
            currentline = _file.readLine().simplified();
            if (dependency.isEmpty())
            {
                appendTempCurve(mat, MAT_CyclicHardeningWithTable, MatProp_Curve_CyclicHardeningWithTable, 2, "循环硬化带表格", dependency);
            }
            else
            {
                appendTempCurve(mat, MAT_CyclicHardeningWithTable, MatProp_Curve_CyclicHardeningWithTable, 3, "循环硬化带表格", dependency);
            }
        }
    }

    void mIMxFile::readDensity(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        MaterialClass matClass = MaterialClass::MAT_Density;
        if (mat->getMaterialType() == AcousticMaterial_Base)
            matClass = MAT_Density_UnCheck;

        currentline = _file.readLine().simplified();
        if (dependency.isEmpty())
        {
            appendMatPropertyValue(mat, matClass, MatProp_Density, currentline);
            currentline = _file.readLine().simplified();
        }
        else
        {
            if (!dependency.compare("temp", Qt::CaseInsensitive))
            {
                mat->addProperty(matClass, MatProp_Refer_X, 0);
            }
            else if (!dependency.compare("freq", Qt::CaseInsensitive))
            {
                mat->addProperty(matClass, MatProp_Refer_X, 1);
            }
            appendTempCurve(mat, matClass, MatProp_Curve_Density, 2, "密度", dependency);
        }
    }

    void mIMxFile::readSoundVelocity(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        appendMatPropertyValue(mat, MAT_SoundVelocity, MatProp_SoundVelocity, currentline);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readBulkModulus(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        appendMatPropertyValue(mat, MAT_BulkModulus, MatProp_BulkModulus, currentline);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readAcousticMaterialDamping(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        appendMatPropertyValue(mat, MAT_AcousticMaterialDamping, MatProp_AcousticMaterialDamping, currentline);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readMaterialDamping(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        appendMatPropertyValue(mat, MAT_MaterialDamping, MatProp_MaterialDamping, currentline);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readExpansion(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString type = getKeyValue(currentline, "type", "=");
        QString dependency = getKeyValue(currentline, "dependency", "=");
        QString reftemp = getKeyValue(currentline, "reftemp", "=");
        if (!reftemp.isEmpty())
        {
            appendMatPropertyValue(mat, MAT_CoefficientOfThermalExpansion, MatProp_ReferenceTemperature, reftemp);
        }

        int size = 1;
        currentline = _file.readLine().simplified();

        if (!type.compare("ortho", Qt::CaseInsensitive))
        {
            mat->addProperty(MAT_CoefficientOfThermalExpansion, MatProp_CoefficientOfThermalExpansion, 1);
            size = 3;
        }
        else if (!type.compare("aniso", Qt::CaseInsensitive))
        {
            mat->addProperty(MAT_CoefficientOfThermalExpansion, MatProp_CoefficientOfThermalExpansion, 2);
            size = 6;
        }
        else if (!type.compare("iso", Qt::CaseInsensitive))
        {
            mat->addProperty(MAT_CoefficientOfThermalExpansion, MatProp_CoefficientOfThermalExpansion, 0);
            size = 1;
        }

        if (dependency.isEmpty())
        {
            _linelist = currentline.split(",");
            if (_linelist.size() < size)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1 热膨胀系数参数不符合规范！").arg(mat->getMaterialName()));
            }
            if (!type.compare("aniso2d", Qt::CaseInsensitive))
            {

                appendMatPropertyValue(mat, MAT_CoefficientOfThermalExpansion, MaterialPropType(MatProp_CoefficientOfThermalExpansion_A1), _linelist[0]);
                appendMatPropertyValue(mat, MAT_CoefficientOfThermalExpansion, MaterialPropType(MatProp_CoefficientOfThermalExpansion_A2), _linelist[1]);
                appendMatPropertyValue(mat, MAT_CoefficientOfThermalExpansion, MaterialPropType(MatProp_CoefficientOfThermalExpansion_A12), _linelist[2]);

            }
            else
            {
                for (int i = 0; i < size; i++)
                {
                    appendMatPropertyValue(mat, MAT_CoefficientOfThermalExpansion, MaterialPropType(i + MatProp_CoefficientOfThermalExpansion_A1), _linelist[i]);
                }
            }

            currentline = _file.readLine().simplified();
        }
        else
        {
            if (!dependency.compare("temp", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_CoefficientOfThermalExpansion, MatProp_Refer_X, 0);
            }
            else if (!dependency.compare("freq", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_CoefficientOfThermalExpansion, MatProp_Refer_X, 1);
            }

            appendTempCurve(mat, MAT_CoefficientOfThermalExpansion, MatProp_Curve_CoefficientOfThermalExpansion, size + 1, "热膨胀系数", dependency);
        }
    }

    void mIMxFile::readHeat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*conductivity", Qt::CaseInsensitive))
            {
                QString type = getKeyValue(currentline, "type", "=");
                QString dependency = getKeyValue(currentline, "dependency", "=");

                int size = 1;
                currentline = _file.readLine().simplified();
                if (!type.compare("ortho", Qt::CaseInsensitive))
                {
                    mat->addProperty(MAT_HeatConductivityCoefficient, MatProp_HeatConductivityCoefficient, 1);
                    size = 3;
                }
                else if (!type.compare("aniso", Qt::CaseInsensitive))
                {
                    mat->addProperty(MAT_HeatConductivityCoefficient, MatProp_HeatConductivityCoefficient, 2);
                    size = 6;
                }
                else
                {
                    mat->addProperty(MAT_HeatConductivityCoefficient, MatProp_HeatConductivityCoefficient, 0);
                    size = 1;
                }

                if (dependency.isEmpty())
                {
                    _linelist = currentline.split(",");
                    if (_linelist.size() < size)
                    {
                        mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1 热传导系数参数不符合规范！").arg(mat->getMaterialName()));
                    }
                    for (int i = 0; i < size; i++)
                    {
                        appendMatPropertyValue(mat, MAT_HeatConductivityCoefficient, MaterialPropType(i + MatProp_HeatConductivityCoefficient_A1), _linelist[i]);
                    }
                    currentline = _file.readLine().simplified();
                }
                else
                {
                    appendTempCurve(mat, MAT_HeatConductivityCoefficient, MatProp_Curve_HeatConductivityCoefficient, size + 1, "热传导系数", dependency);
                }

            }
            else if (currentline.startsWith("*specificheat", Qt::CaseInsensitive))
            {
                QString dependency = getKeyValue(currentline, "dependency", "=");
                currentline = _file.readLine().simplified();
                if (dependency.isEmpty())
                {
                    appendMatPropertyValue(mat, MAT_Heat, MatProp_HeatCapacityDensity, currentline);
                    currentline = _file.readLine().simplified();
                }
                else
                {
                    appendTempCurve(mat, MAT_Heat, MatProp_Curve_HeatCapacityDensity, 2, "热容系数", dependency);
                }
            }
            else if (currentline.startsWith("*heatgeneration", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                appendMatPropertyValue(mat, MAT_HeatGenerationRate, MatProp_HeatProductionRate, currentline);
                currentline = _file.readLine().simplified();
            }
            else
            {
                break;
            }
        }
    }

    void mIMxFile::readFailureEffect(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        QString type = getKeyValue(currentline, "type", "=");
        int typeIdx = type == "iso" ? 0 : 1;
        mat->addProperty(MAT_YieldStrain, MatProp_FailStrainType, typeIdx);
        currentline = _file.readLine().simplified();
        if (dependency.isEmpty())
        {
            _linelist = currentline.split(",");
            if (type == "iso" && _linelist.size() == 3)
            {
                for (int i = 0; i < 3; ++i)
                {
                    appendMatPropertyValue(mat, MAT_YieldStrain, MaterialPropType(i + MatProp_St), _linelist[i]);
                }
            }
            else if (type == "ortho2d" && _linelist.size() == 5)
            {
                for (int i = 0; i < 5; ++i)
                {
                    appendMatPropertyValue(mat, MAT_YieldStrain, MaterialPropType(i + MatProp_Xt), _linelist[i]);
                }
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1 应变失效参数不符合规范！").arg(mat->getMaterialName()));
            }
        }
        else
        {
            if (!dependency.compare("temp", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_YieldStrain, MatProp_Refer_X, 0);
            }
            else if (!dependency.compare("freq", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_YieldStrain, MatProp_Refer_X, 1);
            }

            if (type == "iso")
            {
                appendTempCurve(mat, MAT_YieldStrain, MatProp_Curve_FailStrain, 4, "失效应变", dependency);
            }
            else if (type == "ortho2d")
            {
                appendTempCurve(mat, MAT_YieldStrain, MatProp_Curve_FailStrain, 6, "失效应变", dependency);
            }
        }
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readFailureStress(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        QString type = getKeyValue(currentline, "type", "=");
        int typeIdx = type == "iso" ? 0 : 1;
        mat->addProperty(MAT_YieldStress, MatProp_FailStressType, typeIdx);
        currentline = _file.readLine().simplified();
        if (dependency.isEmpty())
        {
            _linelist = currentline.split(",");
            if (type == "iso" && _linelist.size() == 5)
            {
                for (int i = 0; i < 5; ++i)
                {
                    appendMatPropertyValue(mat, MAT_YieldStress, MaterialPropType(i + MatProp_St), _linelist[i]);
                }
            }
            else if (type == "ortho2d" && _linelist.size() == 7)
            {
                for (int i = 0; i < 7; ++i)
                {
                    appendMatPropertyValue(mat, MAT_YieldStress, MaterialPropType(i + MatProp_Xt), _linelist[i]);
                }
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1 应力失效参数不符合规范！").arg(mat->getMaterialName()));
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            if (!dependency.compare("temp", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_YieldStress, MatProp_Refer_X, 0);
            }
            else if (!dependency.compare("freq", Qt::CaseInsensitive))
            {
                mat->addProperty(MAT_YieldStress, MatProp_Refer_X, 1);
            }

            if (type == "iso")
            {
                appendTempCurve(mat, MAT_YieldStress, MatProp_Curve_FailStress, 6, "失效应力", dependency);
            }
            else if (type == "ortho2d")
            {
                appendTempCurve(mat, MAT_YieldStress, MatProp_Curve_FailStress, 8, "失效应力", dependency);
            }
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readCreep(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        MxFunctions::MaterialClass foldClass = MxFunctions::MaterialClass::MAT_CreepBehavior;
        if (mat->getMaterialType() == Plastic_Normal)
        {
            foldClass = MAT_CreepBehavior_Check;
        }

        QString dependency = getKeyValue(currentline, "dependency", "=");
        QString law = getKeyValue(currentline, "law", "=");
        bool isAnand = !law.compare("anand", Qt::CaseInsensitive);
        if (!law.compare("time", Qt::CaseInsensitive))
        {
            mat->addProperty(foldClass, MatProp_HardeningType, 0);
        }
        else if (!law.compare("strain", Qt::CaseInsensitive))
        {
            mat->addProperty(foldClass, MatProp_HardeningType, 1);
        }
        else if (isAnand)
        {
            mat->addProperty(foldClass, MatProp_HardeningType, 2);
        }

        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        if (dependency.isEmpty())
        {
            if (isAnand)
            {
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_DeformationResistance, getQStringListIndexValue(_linelist, 0));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_ActivationEnergy, getQStringListIndexValue(_linelist, 1));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_PowerLawMultiplier1, getQStringListIndexValue(_linelist, 2));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_StressMultiplier, getQStringListIndexValue(_linelist, 3));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_StressExponent, getQStringListIndexValue(_linelist, 4));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_HardeningConstant, getQStringListIndexValue(_linelist, 5));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_ResistancePowerLawMultiplier, getQStringListIndexValue(_linelist, 6));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_ResistancePowerExponent, getQStringListIndexValue(_linelist, 7));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_RateExponent, getQStringListIndexValue(_linelist, 8));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_TempCoefficientS2, getQStringListIndexValue(_linelist, 9));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_TempCoefficientS3, getQStringListIndexValue(_linelist, 10));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_TempCoefficientA1, getQStringListIndexValue(_linelist, 11));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_TempCoefficientA2, getQStringListIndexValue(_linelist, 12));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_CreepCoefficientA3, getQStringListIndexValue(_linelist, 13));
                appendMatPropertyValue(mat, foldClass, MatProp_Anand_CreepCoefficientA4, getQStringListIndexValue(_linelist, 14));
                currentline = _file.readLine().simplified();
            }
            else
            {
                appendMatPropertyValue(mat, foldClass, MatProp_NortonCoefficient, getQStringListIndexValue(_linelist, 0));
                appendMatPropertyValue(mat, foldClass, MatProp_StressExponent, getQStringListIndexValue(_linelist, 1));
                appendMatPropertyValue(mat, foldClass, MatProp_TimeIndex, getQStringListIndexValue(_linelist, 2));
                currentline = _file.readLine().simplified();
            }
        }
        else
        {
            appendTempCurve(mat, foldClass, MatProp_Curve_CreepBehavior, 4, "本构", dependency);
        }

    }

    void mIMxFile::readCyclichardening(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        QString dependency = getKeyValue(currentline, "dependency", "=");
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        appendMatPropertyValue(mat, MAT_CyclicHardening, MatProp_YieldStress, getQStringListIndexValue(_linelist, 0));
        appendMatPropertyValue(mat, MAT_CyclicHardening, MatProp_HardeningRateControl, getQStringListIndexValue(_linelist, 1));
        //appendTempCurve(mat, MAT_CompressiveHardening, MatProp_Curve_Cyclichardening, 2, "本构", dependency);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::setElementExtraData(int elementId, ElementType elementType, QString currentline)
    {

        switch (elementType)
        {
        case CPDSG:
        case T3:
        case TRIA3:
        case CTRIA3:
        {
            MechElementData::mShellElement* element = new MechElementData::mShellElement(elementType);
            _linelist = currentline.split(",");
            if (_linelist.size() > 4 && !_linelist[4].simplified().isEmpty())
            {
                if (!_linelist[4].compare("\""))
                {
                    element->_paraType = "材料坐标系";
                    _elementToCoorName.insert(elementId);
                    element->_coorName = getQStringListIndexValue(_linelist, 4);
                }
                else
                {
                    element->_paraType = "材料转角值";
                    element->_angle = getQStringListIndexValue(_linelist, 4);
                }
            }
            element->_bias = getQStringListIndexValue(_linelist, 5);
            _modeldata->getElementData()->assignElementToMesh(elementId, element);
            break;
        }
        case CQUAD4:
        case Q4:
        case MITC4:
        {
            MechElementData::mShellElement* element = new MechElementData::mShellElement(elementType);
            _linelist = currentline.split(",");
            if (_linelist.size() > 5 && !_linelist[4].simplified().isEmpty())
            {
                if (!_linelist[5].compare("\""))
                {
                    element->_paraType = "材料坐标系";
                    _elementToCoorName.insert(elementId);
                    element->_coorName = getQStringListIndexValue(_linelist, 5);
                }
                else
                {
                    element->_paraType = "材料转角值";
                    element->_angle = getQStringListIndexValue(_linelist, 5);
                }
            }
            element->_bias = getQStringListIndexValue(_linelist, 6);
            _modeldata->getElementData()->assignElementToMesh(elementId, element);
            break;
        }
        case Mass1:
        {
            MechElementData::mMassElement* element = new MechElementData::mMassElement;
            _linelist = currentline.split(",");
            element->_mass = getQStringListIndexValue(_linelist, 2);
            if (_linelist.size() > 3)
            {
                element->_x = getQStringListIndexValue(_linelist, 3);
                element->_y = getQStringListIndexValue(_linelist, 4);
                element->_z = getQStringListIndexValue(_linelist, 5);
                element->_I11 = getQStringListIndexValue(_linelist, 6);
                element->_I21 = getQStringListIndexValue(_linelist, 7);
                element->_I22 = getQStringListIndexValue(_linelist, 8);
                element->_I31 = getQStringListIndexValue(_linelist, 9);
                element->_I32 = getQStringListIndexValue(_linelist, 10);
                element->_I33 = getQStringListIndexValue(_linelist, 11);
            }
            _modeldata->getElementData()->assignElementToMesh(elementId, element);
            break;
        }
        case CMASS2:
        {
            MechElementData::mCmass2Element* element = new MechElementData::mCmass2Element;
            _linelist = currentline.split(",");
            element->_mass = getQStringListIndexValue(_linelist, 2);
            element->_dof = getQStringListIndexValue(_linelist, 3);
            _modeldata->getElementData()->assignElementToMesh(elementId, element);
            break;
        }
        case D3B2:
        {
            MechElementData::mBeamElement* element = new MechElementData::mBeamElement;
            _linelist = currentline.split(",");
            if (_linelist.size() == 4)
            {
                element->_node = _linelist.at(3).simplified();
            }
            else if (_linelist.size() == 6)
            {
                if (_linelist.at(3).contains("."))
                {
                    element->_beamValue = QString("%1,%2,%3").arg(_linelist.at(3).simplified()).arg(_linelist.at(4).simplified()).arg(_linelist.at(5).simplified());
                }
                else
                {
                    element->_node = _linelist.at(3).simplified();
                    element->_pinsa = _linelist.at(4).simplified();
                    element->_pinsb = _linelist.at(5).simplified();
                }
            }
            else if (_linelist.size() == 8)
            {
                element->_beamValue = QString("%1,%2,%3").arg(_linelist.at(3).simplified()).arg(_linelist.at(4).simplified()).arg(_linelist.at(5).simplified());
                element->_pinsa = _linelist.at(6).simplified();
                element->_pinsb = _linelist.at(7).simplified();
            }
            else if (_linelist.size() == 11)
            {
                element->_node = _linelist.at(3).simplified();
                element->_biasType = _linelist.at(4).simplified();
                element->_X1 = _linelist.at(5).simplified();
                element->_Y1 = _linelist.at(6).simplified();
                element->_Z1 = _linelist.at(7).simplified();
                element->_X2 = _linelist.at(8).simplified();
                element->_Y2 = _linelist.at(9).simplified();
                element->_Z2 = _linelist.at(10).simplified();
            }
            else if (_linelist.size() == 13)
            {
                if (_linelist.at(3).contains("."))
                {
                    element->_beamValue = QString("%1,%2,%3").arg(_linelist.at(3).simplified()).arg(_linelist.at(4).simplified()).arg(_linelist.at(5).simplified());
                    element->_biasType = _linelist.at(6).simplified();
                    element->_X1 = _linelist.at(7).simplified();
                    element->_Y1 = _linelist.at(8).simplified();
                    element->_Z1 = _linelist.at(9).simplified();
                    element->_X2 = _linelist.at(10).simplified();
                    element->_Y2 = _linelist.at(11).simplified();
                    element->_Z2 = _linelist.at(12).simplified();
                }
                else
                {
                    element->_node = _linelist.at(3).simplified();
                    element->_biasType = _linelist.at(4);
                    element->_X1 = _linelist.at(5).simplified();
                    element->_Y1 = _linelist.at(6).simplified();
                    element->_Z1 = _linelist.at(7).simplified();
                    element->_X2 = _linelist.at(8).simplified();
                    element->_Y2 = _linelist.at(9).simplified();
                    element->_Z2 = _linelist.at(10).simplified();
                    element->_pinsa = _linelist.at(11).simplified();
                    element->_pinsb = _linelist.at(12).simplified();
                }
            }
            else if (_linelist.size() == 15)
            {
                element->_beamValue = QString("%1,%2,%3").arg(_linelist.at(3).simplified()).arg(_linelist.at(4).simplified()).arg(_linelist.at(5).simplified());
                element->_biasType = _linelist.at(6).simplified();
                element->_X1 = _linelist.at(7).simplified();
                element->_Y1 = _linelist.at(8).simplified();
                element->_Z1 = _linelist.at(9).simplified();
                element->_X2 = _linelist.at(10).simplified();
                element->_Y2 = _linelist.at(11).simplified();
                element->_Z2 = _linelist.at(12).simplified();
                element->_pinsa = _linelist.at(13).simplified();
                element->_pinsb = _linelist.at(14).simplified();
            }
            _modeldata->getElementData()->assignElementToMesh(elementId, element);
            break;
        }
        case ELAS1:
        case VISCOUSDAMP:
        case DASHPOT2:
        {
            MechElementData::mElas1Element* element = new MechElementData::mElas1Element(elementType);
            _linelist = currentline.split(",");
            element->_dof = getQStringListIndexValue(_linelist, 3);
            _modeldata->getElementData()->assignElementToMesh(elementId, element);
            break;
        }
        case DASHPOT1:
        {
            MechElementData::mElas1Element* element = new MechElementData::mElas1Element(elementType);
            _linelist = currentline.split(",");
            element->_dof = getQStringListIndexValue(_linelist, 2);
            _modeldata->getElementData()->assignElementToMesh(elementId, element);
            break;
        }
        case CBUSH:
        case CBUSH1:
        {
            MechElementData::mCbushElement* element = new MechElementData::mCbushElement(elementType);
            _linelist = currentline.split(",");

            element->_cidName = getQStringListIndexValue(_linelist, 3);
            element->_s = getQStringListIndexValue(_linelist, 4);
            element->_ocidName = getQStringListIndexValue(_linelist, 5);

            element->_x1 = getQStringListIndexValue(_linelist, 6);
            element->_x2 = getQStringListIndexValue(_linelist, 7);
            element->_x3 = getQStringListIndexValue(_linelist, 8);
            _elementToCoorName.insert(elementId);
            _modeldata->getElementData()->assignElementToMesh(elementId, element);
            break;
        }
        default:
            _modeldata->getElementData()->assignElementTypeToMesh(elementId, elementType);
            break;
        }
    }

    void mIMxFile::readCaseLoad(const int& caseID)
    {
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString loadName = getQStringListIndexValue(_linelist, 0);
            loadName = ExistRenamePhysicalName(Physical_LoadBoundary, loadName);
            QString value = getQStringListIndexValue(_linelist, 1);
            if (!value.isEmpty())
            {
                _mapCaseValue[Physical_LoadBoundary][loadName].push_back({ caseID, value.toDouble() });
            }
            else
            {
                _mapCaseValue[Physical_LoadBoundary][loadName].push_back({ caseID, 1.0 });
            }
        }
    }

    void mIMxFile::readOmega(const int& caseID)
    {
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString omega = getQStringListIndexValue(_linelist, 0);
            omega = ExistRenamePhysicalName(Physical_RotateAnalyse, omega);
            _mapCaseValue[Physical_RotateAnalyse][omega].push_back({ caseID, 1.0 });
        }
    }

    void mIMxFile::readCaseNsm(const int& caseID)
    {
        QStringList nsms;
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            QString nsmName = currentline.trimmed();
            if (nsmName.startsWith('"') && nsmName.endsWith('"'))
                nsmName = nsmName.mid(1, nsmName.length() - 2);
            nsmName = ExistRenamePhysicalName(Physical_NonStructuralQuality, nsmName);
            nsms.append(nsmName);
        }
        _caseNsm[caseID] = nsms;
    }

    void mIMxFile::readCaseBC(const int& caseID)
    {
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString bcName = getQStringListIndexValue(_linelist, 0);
            bcName = ExistRenamePhysicalName(Physical_LoadBoundary, bcName);
            QString value = getQStringListIndexValue(_linelist, 1);
            if (!value.isEmpty())
            {
                _mapCaseValue[Physical_LoadBoundary][bcName].push_back({ caseID, value.toDouble() });
            }
            else
            {
                _mapCaseValue[Physical_LoadBoundary][bcName].push_back({ caseID, 1.0 });
            }
        }
    }

    void mIMxFile::readModelChangeData()
    {
        mModelFixManager& manager = _modeldata->getManager<mModelFixManager>(Physical_ModelFix);

        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_ModelFix, name);

        int id = manager.createID();
        std::shared_ptr<mEleSetCtrl> eleSetCtrl = std::make_shared<mEleSetCtrl>(id, name);
        manager.AddModelFixData(eleSetCtrl);

        QString type = getKeyValue(currentline, "type", "=");
        if (type.isEmpty())
            type = "element";
        eleSetCtrl->setSelectSetType(type);

        QString method = getKeyValue(currentline, "method", "=").toLower();
        eleSetCtrl->setActiveOrRemove(method == "active");

        QStringList mergedSetNames;
        currentline = _file.readLine().simplified();
        while (!_file.atEnd() && !currentline.startsWith("*", Qt::CaseInsensitive))
        {
            _linelist = currentline.split(",");
            QString setName = getQStringListIndexValue(_linelist, 0);

            if (!setName.isEmpty())
                mergedSetNames.append(setName);
            currentline = _file.readLine().simplified();
        }

        // 仅单个 set 时立即建立映射；多 set 合并后再统一建立映射
        if (mergedSetNames.size() == 1)
        {
            appendMapPhysicalStruct(Physical_ModelFix, id, Physical_Set, mergedSetNames[0]);
        }
        else
        {
            eleSetCtrl->setSelectSetType(mergedSetNames.join(","));
        }
    }

    void mIMxFile::mergeModelChangeSets()
    {
        mModelFixManager& manager = _modeldata->getManager<mModelFixManager>(Physical_ModelFix);
        std::vector<int> fixIDs = manager.getAllModelFixID();

        for (int fixID : fixIDs)
        {
            std::shared_ptr<mModelFixDataBase> base = manager.getModelFixData(fixID);
            if (base == nullptr)
                continue;

            std::shared_ptr<mEleSetCtrl> eleSetCtrl = std::dynamic_pointer_cast<mEleSetCtrl>(base);
            if (eleSetCtrl == nullptr)
                continue;

            QStringList setNames = eleSetCtrl->getSelectSetType().split(",");
            if (setNames.size() < 2)
                continue;

            // 多个 set：合并去重生成新 element set，名称为 *modelchange 的 name
            std::set<RenderPickResultData> uniqueData;
            for (const QString& setName : setNames)
            {
                QString renamedSetName = ExistRenamePhysicalName(Physical_Set, setName);
                MechSetData::mSetBase* setBase = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(renamedSetName);
                if (setBase == nullptr)
                    continue;

                if (setBase->getSetType() == Element)
                {
                    MechSetData::mElementSet* elementSet = dynamic_cast<MechSetData::mElementSet*>(setBase);
                    if (elementSet != nullptr)
                    {
                        for (const RenderPickResultData& data : elementSet->getElementSetData())
                            uniqueData.insert(data);
                    }
                }
            }

            if (uniqueData.empty())
                continue;

            // 创建新的 element set
            int newSetID = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).createID();
            QString newSetName = eleSetCtrl->getModelFixName();
            newSetName = ExistRenamePhysicalName(Physical_Set, newSetName);
            MechSetData::mElementSet* newSet = new MechSetData::mElementSet(newSetID, Element, newSetName, false, true);
            getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(newSet);

            std::vector<RenderPickResultData> mergedData(uniqueData.begin(), uniqueData.end());
            newSet->setElementSetData(mergedData);

            _modeldata->getMapRelationManager().appendMapRelation(
                { Physical_ModelFix, fixID },
                { Physical_Set, newSetID });
        }
    }

    void mIMxFile::readInitTempData()
    {
        QString setName = getKeyValue(currentline, "set", "=");
        QString fieldName = getKeyValue(currentline, "name", "=");

        int fieldID = _modeldata->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).createID();
        fieldName = ExistRenamePhysicalName(Physical_PredefinedField, fieldName);
        std::shared_ptr<MechCaseData::mPredefinedFieldInitTemp> initTemp = std::make_shared<MechCaseData::mPredefinedFieldInitTemp>(fieldID, PREDEFINED_FIELD_INIT_TEMP, fieldName);

        // 读取*ibc行的下一行
        currentline = _file.readLine().simplified();

        // 核心区分逻辑：按*ibc行是否包含set参数
        if (!setName.isEmpty())
        {
            // ========== 统一赋值模式 ==========
            initTemp->_defineType = 0;

            if (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                //温度仿射换算：文件值(当前界面单位制) → 内部值（非数值原样保留）
                bool tempOk = false; double tempVal = currentline.toDouble(&tempOk);
                initTemp->_temp = tempOk ? MxFunctions::mNumberToString(mUnitIoCodec::decodeTemp(tempVal)) : currentline;
                currentline = _file.readLine().simplified();
            }

            appendMapPhysicalStruct(Physical_PredefinedField, fieldID, Physical_Set, setName);
        }
        else
        {
            // ========== 离散（独立）赋值模式 ==========
            initTemp->_defineType = 1;
            initTemp->_nodeTempMap.clear();

            while (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                // 按逗号分割生成行数据列表
                _linelist = currentline.split(',');

                // 使用指定函数获取索引对应的值
                QString nodeIdStr = getQStringListIndexValue(_linelist, 0).trimmed();
                QString tempStr = getQStringListIndexValue(_linelist, 1).trimmed();

                int nodeId = nodeIdStr.toInt() + _lastMaxNodeID;
                RenderPickResultData key(nodeId, -1, -1);
                //温度仿射换算：文件值(当前界面单位制) → 内部值（非数值原样保留）
                bool tempOk = false; double tempVal = tempStr.toDouble(&tempOk);
                QString storeStr = tempOk ? MxFunctions::mNumberToString(mUnitIoCodec::decodeTemp(tempVal)) : tempStr;
                initTemp->_nodeTempMap[key] = QVector<QVariant>{ storeStr };
                currentline = _file.readLine().simplified();
            }
        }

        _modeldata->addPhysicalFieldP(initTemp);
    }

    void mIMxFile::readTempLoadData()
    {
        QString setName = getKeyValue(currentline, "set", "=");
        QString fieldName = getKeyValue(currentline, "name", "=");
        QString filePath = getKeyValue(currentline, "file", "=");
        QString typeStr = getKeyValue(currentline, "type", "=");
        QString midNodeStr = getKeyValue(currentline, "midside", "=");
        QString funcName = getKeyValue(currentline, "function", "=");

        int fieldID = _modeldata->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).createID();
        fieldName = ExistRenamePhysicalName(Physical_PredefinedField, fieldName);
        std::shared_ptr<MechCaseData::mPredefinedFieldTempLoad> tempLoad =
            std::make_shared<MechCaseData::mPredefinedFieldTempLoad>(fieldID, PREDEFINED_FIELD_TEMP_LOAD, fieldName);

        if (!filePath.isEmpty())
        {
            // ========== 模式2：文件定义 ==========
            tempLoad->_defineType = 2;
            tempLoad->_filePath = filePath;

            if (!typeStr.compare("single-step", Qt::CaseInsensitive))
                tempLoad->_interpolateType = 0;
            else if (!typeStr.compare("multi-step", Qt::CaseInsensitive))
                tempLoad->_interpolateType = 1;

            if (!midNodeStr.compare("on", Qt::CaseInsensitive))
                tempLoad->_midNodeInterpolate = true;

            // 文件定义没有下一行数据，直接读取下一个命令
            currentline = _file.readLine().simplified();
        }
        else if (!setName.isEmpty())
        {
            // ========== 模式0：统一赋值 ==========
            tempLoad->_defineType = 0;

            currentline = _file.readLine().simplified();
            if (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                //温度仿射换算：文件值(当前界面单位制) → 内部值（非数值原样保留）
                bool tempOk = false; double tempVal = currentline.toDouble(&tempOk);
                tempLoad->_temp = tempOk ? MxFunctions::mNumberToString(mUnitIoCodec::decodeTemp(tempVal)) : currentline;
                currentline = _file.readLine().simplified();
            }

            appendMapPhysicalStruct(Physical_PredefinedField, fieldID, Physical_Set, setName);
        }
        else
        {
            // ========== 模式1：独立赋值 ==========
            tempLoad->_defineType = 1;
            tempLoad->_nodeTempMap.clear();

            // 循环读取所有节点温度行
            currentline = _file.readLine().simplified();
            while (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                QStringList _linelist = currentline.split(',');
                QString nodeIdStr = getQStringListIndexValue(_linelist, 0).trimmed();
                QString tempStr = getQStringListIndexValue(_linelist, 1).trimmed();

                int nodeId = nodeIdStr.toInt() + _lastMaxNodeID;
                RenderPickResultData key(nodeId, -1, -1);
                //温度仿射换算：文件值(当前界面单位制) → 内部值（非数值原样保留）
                bool tempOk = false; double tempVal = tempStr.toDouble(&tempOk);
                QString storeStr = tempOk ? MxFunctions::mNumberToString(mUnitIoCodec::decodeTemp(tempVal)) : tempStr;
                tempLoad->_nodeTempMap[key] = QVector<QVariant>{ storeStr };
                currentline = _file.readLine().simplified();
            }
        }

        //保存函数映射
        if (!funcName.isEmpty())
        {
            appendMapPhysicalStruct(Physical_PredefinedField, fieldID, Physical_Function, funcName);
        }

        _modeldata->addPhysicalFieldP(tempLoad);
    }

    void mIMxFile::readCaseInitTemp(const int& caseID)
    {
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString fieldName = getQStringListIndexValue(_linelist, 0);
            fieldName = ExistRenamePhysicalName(Physical_PredefinedField, fieldName);
            _mapCaseValue[Physical_PredefinedField][fieldName].push_back({ caseID, 1.0 });
        }
    }

    void mIMxFile::readCaseTempLoadField(const int& caseID)
    {
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString fieldName = getQStringListIndexValue(_linelist, 0);
            fieldName = ExistRenamePhysicalName(Physical_PredefinedField, fieldName);
            _mapCaseValue[Physical_PredefinedField][fieldName].push_back({ caseID, 1.0 });
        }
    }

    void mIMxFile::readCaseInitBug(const int& caseID)
    {
        MechCaseData::mCaseBase* caseBase = _modeldata->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
        if (caseBase == nullptr) return;
        MechCaseData::mCaseImplicitNonLinearArc* arcCase = dynamic_cast<MechCaseData::mCaseImplicitNonLinearArc*>(caseBase);
        if (arcCase == nullptr) return;

        QString type = getKeyValue(currentline, "type", "=");
        QString file = getKeyValue(currentline, "file", "=");
        QString scale = getKeyValue(currentline, "scale", "=");

        arcCase->_initBug = true;
        if (!type.compare("static", Qt::CaseInsensitive))
        {
            arcCase->_initBugType = MxFunctions::InitBugType::Static;
        }
        else
        {
            arcCase->_initBugType = MxFunctions::InitBugType::Buckling;
        }
        arcCase->_initBugFileName = file;
        arcCase->_scaleValue = scale.isEmpty() ? "1.0" : scale;

        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }
        }
    }

    void mIMxFile::readCaseContactinterference(const int& caseID)
    {
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString contactName = getQStringListIndexValue(_linelist, 0);
            contactName = ExistRenamePhysicalName(Physical_Contact, contactName);
            _mapCaseValue[Physical_Contact][contactName].push_back({ caseID, 1.0 });
        }
    }

    void mIMxFile::readCaseModelFix(const int& caseID)
    {
        vector<QString> modelFixs;
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString modelFixName = getQStringListIndexValue(_linelist, 0);
            modelFixName = ExistRenamePhysicalName(Physical_ModelFix, modelFixName);
            modelFixs.push_back(modelFixName);
        }
        appendMapPhysicalTable(Physical_Case, caseID, Physical_ModelFix, modelFixs);
    }
    void mIMxFile::readCasePchOutPut(const int& caseID)
    {
        MechCaseData::mCaseBase* caseBase = _modeldata->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
        if (caseBase == nullptr) return;
        QString type = "";
        if (currentline.startsWith("*output")) {
            QStringList types = currentline.split("=");
            if (types.size() > 1) {
                type = types[1];
            }
        }
        if (type != "pch") {
            return;
        }

        pair<QString, QString> key;
        QHash<QString, QStringList> outputs;
        QHash<QString, QHash<QString, QStringList>> selectedSet;
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString var = getQStringListIndexValue(_linelist, 0);
            QStringList selectedsets;
            for (QString setname : _linelist.mid(1)) {
                selectedsets.append(setname.remove("\""));
            }
            key = mVariableFunction::getOutPutPairByVariable(var);
            outputs[key.first].append(key.second);
            selectedSet[key.first].insert(key.second, selectedsets);
        }
        caseBase->setPchOutputs(outputs);
        caseBase->setPchSelectedSet(selectedSet);
    }

    void mIMxFile::readCaseOutPut(const int& caseID)
    {
        MechCaseData::mCaseBase* caseBase = _modeldata->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
        if (caseBase == nullptr) return;
        QString type = "mxdb";
        if (currentline.startsWith("*output")) {
            QStringList types = currentline.split("=");
            if (types.size() > 1) {
                type = types[1];
            }
        }
        if (type == "pch") {
            readCasePchOutPut(caseID);
            return;
        }
        caseBase->setOutputType(type);

        pair<QString, QString> key;
        QHash<QString, QStringList> outputs;
        QHash<QString, QHash<QString, QStringList>> selectedSet;
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString var = getQStringListIndexValue(_linelist, 0);
            QStringList selectedsets;
            for (QString setname : _linelist.mid(1)) {
                selectedsets.append(setname.remove("\""));
            }
            key = mVariableFunction::getOutPutPairByVariable(var);
            outputs[key.first].append(key.second);
            selectedSet[key.first].insert(key.second, selectedsets);
        }
        caseBase->setOutputs(outputs);
        caseBase->setSelectedSet(selectedSet);
    }

    void mIMxFile::readGlobalOutputControl()
    {
        if (!_modeldata)
        {
            return;
        }

        auto globalParam = _modeldata->getGlobalParam();
        if (!globalParam)
        {
            return;
        }

        if (!globalParam->getGlobalOutputControlData())
        {
            globalParam->createGlobalOutputControlData();
        }

        auto data = globalParam->getGlobalOutputControlData();
        if (!data)
        {
            return;
        }

        QHash<QString, QStringList> outputs;
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break;
            }
            else if (currentline.isEmpty())
            {
                continue;
            }

            _linelist = currentline.split(",");
            QString var = getQStringListIndexValue(_linelist, 0);
            auto key = mVariableFunction::getOutPutPairByVariable(var);
            if (!key.first.isEmpty())
            {
                outputs[key.first].append(key.second);
            }
        }

        data->setOutputSelections(outputs);
    }

    void mIMxFile::readHistoryOutput(const int& caseID)
    {
        MechCaseData::mCaseBase* caseBase = _modeldata->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
        if (caseBase == nullptr) return;

        // 英文关键字 -> 中文名称反向映射表（与写入时完全对应）
        static const QHash<QString, QString> reverseKeywordMap = {
            {"disp", "位移"},
            {"vel", "速度"},
            {"acc", "加速度"},
            {"strain", "应变"},
            {"estrain", "单元应变"},
            {"stress", "应力"},
            {"estress", "单元应力"},
            {"temperature", "温度"},
            {"rf", "反力"}
        };

        // 1. 解析*history第一行的参数（使用你的getKeyValue工具函数）
        currentline = currentline.simplified();
        caseBase->setHistoryOutputEnabled(true); // 读到*history关键字，自动启用历史输出

        // 直接使用getKeyValue获取各个参数，自动处理空格和引号
        QString filetype = getKeyValue(currentline, "filetype", "=");
        QString type = getKeyValue(currentline, "type", "=");
        QString setName = getKeyValue(currentline, "set", "=");

        appendMapPhysicalStruct(Physical_Case, caseID, Physical_Set, setName, MapRelation_HistorySet);

        if (!filetype.isEmpty()) {
            caseBase->setHstFileType(filetype);
        }

        if (!type.isEmpty()) {
            // 求解器关键字 -> 界面中文转换
            if (type == "real")
                caseBase->setHstDataType("实数");
            else if (type == "imag")
                caseBase->setHstDataType("复数");
            else if (type == "mag")
                caseBase->setHstDataType("幅值");
            else if (type == "phase")
                caseBase->setHstDataType("相位");
            else if (type == "both")
                caseBase->setHstDataType("全部");
        }

        // 2. 读取后续输出项（完全和readCaseOutPut一样的循环逻辑）
        QHash<QString, QStringList> historyOutputs;
        QStringList kdOutputs;
        QString dynamicStiff;

        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("*"))
            {
                break; // 遇到下一个关键字，直接break，主循环会处理这一行
            }
            else if (currentline.isEmpty())
            {
                continue;
            }
            _linelist = currentline.split(",");
            //获取第一个值
            QString firstPart = getQStringListIndexValue(_linelist, 0);

            if (firstPart == "Kd") {
                // 处理动刚度结果
                if (_linelist.size() < 2) continue;

                //获取方法名
                QString method = getQStringListIndexValue(_linelist, 1);
                if (method == "average") {
                    kdOutputs.append("平均法");
                }
                else if (method == "index") {
                    kdOutputs.append("指数法");
                }
                else if (method == "area") {
                    kdOutputs.append("面积法");
                    // 提取动刚度值（如果有），同样使用你的工具函数
                    if (_linelist.size() >= 3) {
                        dynamicStiff = getQStringListIndexValue(_linelist, 2);
                    }
                }
            }
            else {
                // 处理常规输出项（基本结果/应变/应力）
                if (!reverseKeywordMap.contains(firstPart)) continue;

                QString chineseName = reverseKeywordMap[firstPart];
                QString category;

                // 自动分类（与UI中的分类完全一致）
                if (chineseName == "位移" || chineseName == "速度" ||
                    chineseName == "加速度" || chineseName == "温度" ||
                    chineseName == "反力") {
                    category = "基本结果";
                }
                else if (chineseName == "应变" || chineseName == "单元应变") {
                    category = "应变";
                }
                else if (chineseName == "应力" || chineseName == "单元应力") {
                    category = "应力";
                }

                if (!category.isEmpty()) {
                    historyOutputs[category].append(chineseName);
                }
            }
        }

        // 3. 将解析结果存入caseBase
        caseBase->setHistoryOutputs(historyOutputs);
        caseBase->setHistoryKdOutputs(kdOutputs);

        if (!dynamicStiff.isEmpty()) {
            caseBase->setHstDynamicStiff(dynamicStiff);
        }
    }

    void mIMxFile::setProgressBarValue(int pos)
    {
        double num = (double)pos / (double)_fileSize;
        quint64 tempNum = num * readFileTimeRatio;
        int value = tempNum;
        mGlobalSignals::getInstance()->setProgressBarValue(value);
    }
    void mIMxFile::analysisMapRelation()
    {
        boltloadMap();
        contactMap();

        MechMapRelation::mMapRelationManager& manager = _modeldata->getMapRelationManager();
        for (auto phyTypeIt = _mapPhysicalStruct.begin(); phyTypeIt != _mapPhysicalStruct.cend(); ++phyTypeIt)
        {
            for (auto phyIDIt = phyTypeIt.value().begin(); phyIDIt != phyTypeIt.value().cend(); ++phyIDIt)
            {
                for (auto resultTypeIt = phyIDIt.value().begin(); resultTypeIt != phyIDIt.value().cend(); ++resultTypeIt)
                {
                    for (auto it = resultTypeIt.value().begin(); it != resultTypeIt.value().cend(); ++it)
                    {
                        int phyID2 = _modeldata->getPhyIDByName(resultTypeIt.key(), it.value(), true);
                        if (phyID2 == -1)
                        {
                            continue;
                        }
                        if (resultTypeIt.key() == Physical_Set)
                            transformMixSet(phyID2);

                        manager.appendMapRelation({ phyTypeIt.key(),  phyIDIt.key() }, { resultTypeIt.key(), phyID2 }, it.key());
                    }
                }
            }
        }

        //用户自建集合卡位于接触卡之后（集合走全局段后置写出）：读接触时按集合名判定
        //slave_region 会因集合尚未登记而漏判（下拉框误显"单元面"），此处按从面集合真实类型统一回填
        rebuildNodeToSurfaceSlaveRegion();

        for (auto it1 = _mapVecPhysical.begin(); it1 != _mapVecPhysical.cend(); ++it1)
        {
            PhysicalFieldType phyType1 = it1.key();
            for (auto itt1 = it1.value().begin(); itt1 != it1.value().cend(); ++itt1)
            {
                int phyID1 = itt1.key();
                for (auto it2 = itt1.value().begin(); it2 != itt1.value().cend(); ++it2)
                {
                    PhysicalFieldType phyType2 = it2.key();
                    int row = 0;
                    for (auto& itt2 : it2.value())
                    {
                        int phyID2 = _modeldata->getPhyIDByName(phyType2, itt2, true);
                        if (phyID2 == -1)
                        {
                            //mGlobalSignals::getInstance()->outputMessageSig(QString("该数据 %1 错误！").arg(itt2));
                            continue;
                        }
                        manager.appendTableMapRelation({ phyType1, phyID1 }, { phyType2, phyID2 }, row++);
                    }
                }
            }
        }

        for (auto elementSurface = _elementSurfaceToElement.begin(); elementSurface != _elementSurfaceToElement.cend(); ++elementSurface)
        {
            MechSetData::mSetBase* setBase = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(elementSurface.key());
            MechSetData::mElementSurface* surface = dynamic_cast<MechSetData::mElementSurface*>(setBase);
            if (surface == nullptr)
                continue;

            vector<RenderPickResultData> datas;
            if (setBase->getSetType() == ElementSurface)
            {
                for (auto it = elementSurface.value().begin(); it != elementSurface.value().cend(); ++it)
                {
                    MechSetData::mElementSet* element = dynamic_cast<MechSetData::mElementSet*>(_modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(it.key()));
                    if (element == nullptr)
                        continue;

                    for (RenderPickResultData data : element->getElementSetData())
                    {
                        data.result_id2 = it.value();
                        datas.push_back(data);
                    }
                }
            }
            else if (setBase->getSetType() == ElementEdge)
            {
                for (auto it = elementSurface.value().begin(); it != elementSurface.value().cend(); ++it)
                {
                    MechSetData::mElementSet* element = dynamic_cast<MechSetData::mElementSet*>(_modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(it.key()));
                    if (element == nullptr)
                        continue;

                    _meshData->getEdgeNodeByPartElementBN(datas, element->getElementSetData(), it.value());
                }
            }

            surface->setElementSurfaceData(datas);
        }

        for (auto nodesurfaceId = m_NodesurfaceHash.begin(); nodesurfaceId != m_NodesurfaceHash.cend(); ++nodesurfaceId)
        {
            MechSetData::mSetBase* setBase = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(nodesurfaceId.key());
            MechSetData::mNodeFace* surface = dynamic_cast<MechSetData::mNodeFace*>(setBase);
            if (surface == nullptr)
                continue;

            if (setBase->getSetType() == NodeSurface)
            {
                vector<RenderPickResultData> datas;
                for(auto name: nodesurfaceId.value())
                {
                    auto node = dynamic_cast<MechSetData::mNode*>(_modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(name));
                    if (node != nullptr)
                    {
                        for (RenderPickResultData data : node->getNodeData())
                        {
                            datas.push_back(data);
                        }
                    }
                }
                surface->setNodeData(datas);
            }
        }

        for (auto typeItem = _mapCaseValue.begin(); typeItem != _mapCaseValue.cend(); ++typeItem)
        {
            PhysicalFieldType type = typeItem.key();
            for (auto it = typeItem.value().begin(); it != typeItem.value().cend(); ++it)
            {
                int phyID = _modeldata->getPhyIDByName(type, it.key());
                if (phyID == -1)
                    continue;
                for (auto caseData : it.value())
                {
                    manager.appendMapRelation({ type, phyID }, { Physical_Case, caseData.first }, MapRelation_None, caseData.second);
                }
            }
        }

        for (auto it = _coorToNodes.begin(); it != _coorToNodes.cend(); ++it)
        {
            MechCoordinateData::mCoordinateBase* setBase = _modeldata->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(it.key());
            if (setBase == nullptr)
                continue;

            setBase->setOrigin(_meshData->getVectorById(it.value()[0]));
            setBase->setXPoint(_meshData->getVectorById(it.value()[1]));
            setBase->setXYPoint(_meshData->getVectorById(it.value()[2]));
        }

        for (auto& it : _elementToCoorName)
        {
            MechElementData::mElementBase* elementBase = _modeldata->getElementData()->getElementDataByMeshID(it);
            if (elementBase == nullptr)
                continue;

            switch (elementBase->getElementType())
            {
            case CPDSG:
            case T3:
            case TRIA3:
            case CTRIA3:
            case CQUAD4:
            case Q4:
            case MITC4:
            {
                MechElementData::mShellElement* shellElement = dynamic_cast<MechElementData::mShellElement*>(elementBase);
                if (shellElement == nullptr) break;

                int phyID = _modeldata->getPhyIDByName(Physical_Coordinate, shellElement->_coorName);
                if (phyID != -1)
                {
                    shellElement->_coordID = phyID;
                }
                break;
            }
            case CBUSH:
            {
                MechElementData::mCbushElement* cbushElement = dynamic_cast<MechElementData::mCbushElement*>(elementBase);
                if (cbushElement == nullptr) break;

                int phyID = _modeldata->getPhyIDByName(Physical_Coordinate, cbushElement->_cidName);
                if (phyID != -1)
                {
                    cbushElement->_cid = phyID;
                }

                phyID = _modeldata->getPhyIDByName(Physical_Coordinate, cbushElement->_ocidName);
                if (phyID != -1)
                {
                    cbushElement->_ocid = phyID;
                }
                break;
            }
            default:
                break;
            }

        }

        analysisRandomCaseOutPut();
        analysisPanleElementMap();
        caseNsmMap();
        mergeModelChangeSets();
        nonElementSetMap();
        nodeCoordMap();
        if (_modeldata->getStructOptionBase())
        {
            for (auto& it : _modeldata->getStructOptionBase()->_designData)
            {
                MechStructOptionData::mOptimizationDesignData* design = it.second;
                if (design->_originId != -1) design->_origin = _meshData->getVectorById(design->_originId);
                if (design->_dirPoint1Id != -1) design->_dirPoint1 = _meshData->getVectorById(design->_dirPoint1Id);
                if (design->_dirPoint2Id != -1) design->_dirPoint2 = _meshData->getVectorById(design->_dirPoint2Id);
                if (design->_startPointId != -1) design->_startPoint = _meshData->getVectorById(design->_startPointId);
                if (design->_dirPointId != -1) design->_dirPoint = _meshData->getVectorById(design->_dirPointId);
                if (design->_supportStartPointId != -1) design->_supportStartPoint = _meshData->getVectorById(design->_supportStartPointId);
                if (design->_supportDirPointId != -1) design->_supportDirPoint = _meshData->getVectorById(design->_supportDirPointId);
                if (design->_repBasePointId != -1) design->_repBasePoint = _meshData->getVectorById(design->_repBasePointId);
                if (design->_repDirPoint1Id != -1) design->_repDirPoint1 = _meshData->getVectorById(design->_repDirPoint1Id);
                if (design->_repDirPoint2Id != -1) design->_repDirPoint2 = _meshData->getVectorById(design->_repDirPoint2Id);
                if (design->_repDirPoint3Id != -1) design->_repDirPoint3 = _meshData->getVectorById(design->_repDirPoint3Id);
            }
        }

        // 解析铺层厚度离散场，设置 Shell 类型属性的 _thickness 下拉框
        for (auto it = _tempPlyThickFieldNames.begin(); it != _tempPlyThickFieldNames.end(); ++it)
        {
            int propID = it.key();
            QString fieldName = it.value();
            int fieldID = _modeldata->getPhyIDByName(Physical_Field, fieldName, true);
            if (fieldID == -1)
                continue;

            MechPropertyData::mPropertyBase* propBase = _modeldata->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(propID);
            MechPropertyData::mPlyLaminate* plyLam = dynamic_cast<MechPropertyData::mPlyLaminate*>(propBase);
            if (plyLam == nullptr || plyLam->_type != 0) // 仅 Shell 类型有厚度离散场下拉框
                continue;

            MechDiscreteFieldData::mDiscreteField* dfData = _modeldata->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field).getDiscreteFieldData(fieldID);
            if (dfData == nullptr)
                continue;

            if (dfData->getLocate() == MxFunctions::PickDataType::Element)
                plyLam->_thickness = "单元厚度离散场";
            else if (dfData->getLocate() == MxFunctions::PickDataType::Node)
                plyLam->_thickness = "节点厚度离散场";
        }
        _tempPlyThickFieldNames.clear();

        // 导入完成后，对启用了转子动力学的复模态分析工况触发树节点刷新
        notifyRotorSettingChangedForImportedCases();
    }

    void mIMxFile::notifyRotorSettingChangedForImportedCases()
    {
        auto& caseManager = _modeldata->getManager<MechCaseData::mCaseManager>(Physical_Case);
        std::vector<int> caseIDs = caseManager.getAllCaseID();
        for (int caseID : caseIDs)
        {
            MechCaseData::mCaseBase* caseBase = caseManager.getCaseData(caseID);
            if (caseBase == nullptr || caseBase->getCaseType() != ImplicitModal)
                continue;

            MechCaseData::mCaseImplicitModal* modalData = dynamic_cast<MechCaseData::mCaseImplicitModal*>(caseBase);
            if (modalData == nullptr)
                continue;

            if (modalData->_modalType == "复模态分析" && modalData->_rotationalChecked)
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->caseRotorSettingChangedSig(caseID);
            }
        }
    }
    QVariant mIMxFile::appendTempCurve(std::shared_ptr<MechMaterialData::mMaterialBase> mat,
        const MaterialClass& matType, const MaterialPropType& propType,
        int column, const QString& errorName, const QString& dependency, bool flag)
    {
        MechMaterialData::CurveType curType = MechMaterialData::CurveType::NoCorrelation;
        if (!dependency.compare("temp", Qt::CaseInsensitive))
        {
            curType = MechMaterialData::CurveType::TemperatureCorrelation;
        }
        else if (!dependency.compare("freq", Qt::CaseInsensitive))
        {
            curType = MechMaterialData::CurveType::FrequencyDependence;
        }

        QVector<QVector<double>> curve;
        curve.resize(column);
        //单位制换算：曲线y列按材料属性量纲（x列为温度/频率，文件格式语义，不换算）
        const MxFunctions::Unit propUnit = mUnitIoCodec::unitOfMaterialProp(propType);
        while (1)
        {
            // 跳过空行，避免空行被当作数据行导致提前结束
            if (!_file.atEnd() && currentline.isEmpty())
            {
                currentline = _file.readLine().simplified();
                continue;
            }

            //判断终止条件
            if (currentline.contains("*") || _file.atEnd())
            {
                QVariant curse = MechMaterialData::mMaterialBase::transCurve2Varint(curve, curType);
                if (flag)
                {
                    return curse;
                }
                else
                {
                    mat->addProperty(matType, propType, MechMaterialData::mMaterialBase::transCurve2Varint(curve, curType));
                    break;
                }
            }

            _linelist = currentline.split(",");
            if (_linelist.size() == column)
            {
                if (curType == MechMaterialData::CurveType::TemperatureCorrelation || curType == MechMaterialData::CurveType::FrequencyDependence)
                {
                    for (int i = 0; i < column; i++)
                    {
                        if (i == column - 1)
                        {
                            curve[0].push_back(_linelist[i].toDouble());
                        }
                        else
                        {
                            curve[i + 1].push_back(mUnitIoCodec::decode(propUnit, _linelist[i].toDouble()));
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < column; i++)
                    {
                        curve[i].push_back(_linelist[i].toDouble());
                    }
                }
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("材料 %1  %2参数不符合规范！").arg(mat->getMaterialName()).arg(errorName));
                break;
            }

            currentline = _file.readLine().simplified();
        }
        return QVariant();
    }

    void mIMxFile::appendMatPropertyValue(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const MaterialClass& matType, const MaterialPropType& propType, const QString& value, MechCommon::QVariantType varType)
    {
        QVariant variant;
        if (!value.simplified().isEmpty())
        {
            switch (varType)
            {
            case MechCommon::QVariantType::QVariant_Int:
                variant = value.toInt();
                break;
            case MechCommon::QVariantType::QVariant_Double:
                //单位制换算：文件值(当前界面单位制) → 内部mm-t-N-s（按材料属性量纲）
                variant = mUnitIoCodec::decode(mUnitIoCodec::unitOfMaterialProp(propType), value.toDouble());
                break;
            default:
                break;
            }
        }
        mat->addProperty(matType, propType, variant);
    }
    void mIMxFile::readTorsionBar(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mTorsionBar* property = new MechPropertyData::mTorsionBar(phyID, RodProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        property->_J = getQStringListIndexValue(_linelist, 0);
        property->_A = getQStringListIndexValue(_linelist, 1);
        property->_nsm = getQStringListIndexValue(_linelist, 2);
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readShearplatesection(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mShearPlate* property = new MechPropertyData::mShearPlate(phyID, ShearPlateProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        property->_thickness = getQStringListIndexValue(_linelist, 0);
        property->_valF1 = getQStringListIndexValue(_linelist, 1);
        property->_valF2 = getQStringListIndexValue(_linelist, 2);
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readTubesection(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mTube* property = new MechPropertyData::mTube(phyID, TubeProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        property->_od = getQStringListIndexValue(_linelist, 0);
        property->_t = getQStringListIndexValue(_linelist, 1);
        property->_nsm = getQStringListIndexValue(_linelist, 2);
        property->_od2 = getQStringListIndexValue(_linelist, 3);
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readShellsection(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mShellHomo* property = new MechPropertyData::mShellHomo(phyID, ShellProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        property->_thickness = getQStringListIndexValue(_linelist, 0);
        property->_nsm = getQStringListIndexValue(_linelist, 1);

        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*referenceplane", Qt::CaseInsensitive))
        {
            QString type = getKeyValue(currentline, "type", "=");
            if (!type.compare("zoffs", Qt::CaseInsensitive))
            {
                property->_referenceplane = "偏置距离";
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                property->_z0 = getQStringListIndexValue(_linelist, 0);
            }
            else if (!type.compare("bottom", Qt::CaseInsensitive))
            {
                property->_referenceplane = "底部";
            }
            else if (!type.compare("top", Qt::CaseInsensitive))
            {
                property->_referenceplane = "顶部";
            }
            currentline = _file.readLine().simplified();
        }
    }
    void mIMxFile::readBeamsection(const int& phyID, const QString& propertyname)
    {
        QString type = getKeyValue(currentline, "type", "=");
        vector<int> shapeIDs; vector<double> xbs;

        if (currentline.startsWith("*pbeamsection", Qt::CaseInsensitive))
        {
            shapeIDs.push_back(createSectionShape(type, xbs));
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*pbeamsectioni", Qt::CaseInsensitive))
                {
                    shapeIDs.push_back(createSectionShape(type, xbs, true));
                }
                else
                {
                    break;
                }
            }
        }
        else if (currentline.startsWith("*beamsection", Qt::CaseInsensitive))
        {
            shapeIDs.push_back(createSectionShape(type, xbs));
        }


        if (shapeIDs.size() > 1)
        {
            MechPropertyData::mCrossBeam* property = new MechPropertyData::mCrossBeam(phyID, CrossProperty, propertyname);
            _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
            property->_paraList.clear();
            int row = 0;
            for (int i = 0; i < shapeIDs.size(); i++)
            {
                property->_paraList.append(MxFunctions::mNumberToString(xbs[i]));
                _modeldata->getMapRelationManager().appendTableMapRelation({ Physical_Property, phyID }, { Physical_PropertyShape ,shapeIDs[i] }, row++);
            }
        }
        else
        {
            MechPropertyData::mConstantBeam* property = new MechPropertyData::mConstantBeam(phyID, ResultantProperty, propertyname);
            _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
            _modeldata->getMapRelationManager().appendMapRelation({ Physical_Property, phyID }, { Physical_PropertyShape ,shapeIDs[0] });
        }
    }
    void mIMxFile::readSpringsection(const int& phyID, const QString& propertyname)
    {
        QString type = getKeyValue(currentline, "type", "=");
        QString NONLINEAR = getKeyValue(currentline, "NONLINEAR", "=");
        if (!type.compare("elas1", Qt::CaseInsensitive))
        {
            MechPropertyData::mSpring* property = new MechPropertyData::mSpring(phyID, Elas1Property, propertyname);
            _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            property->_K = getQStringListIndexValue(_linelist, 0);
            property->_GE = getQStringListIndexValue(_linelist, 1);
            property->_S = getQStringListIndexValue(_linelist, 2);
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("SPRING1", Qt::CaseInsensitive) || !type.compare("SPRING2", Qt::CaseInsensitive))
        {
            MechPropertyData::mSpring* property = new MechPropertyData::mSpring(phyID, Spring1Property, propertyname);
            _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
            if (!type.compare("SPRING2", Qt::CaseInsensitive)) property->setPropertyType(Spring2Property);

            currentline = _file.readLine().simplified();
            if (currentline.contains("1", Qt::CaseInsensitive))
            {
                property->_k = "x";
            }
            else if (currentline.contains("2", Qt::CaseInsensitive))
            {
                property->_k = "y";
            }
            else if (currentline.contains("3", Qt::CaseInsensitive))
            {
                property->_k = "z";
            }
            else if (currentline.contains("4", Qt::CaseInsensitive))
            {
                property->_k = "rx";
            }
            else if (currentline.contains("5", Qt::CaseInsensitive))
            {
                property->_k = "ry";
            }
            else if (currentline.contains("6", Qt::CaseInsensitive))
            {
                property->_k = "rz";
            }

            currentline = _file.readLine().simplified();
            if (!NONLINEAR.compare("on", Qt::CaseInsensitive))
            {
                property->_nonlinear = "非线性";
                while (!_file.atEnd())
                {
                    if (currentline.startsWith("*")) break;

                    _linelist = currentline.split(",");
                    double load = getQStringListIndexValue(_linelist, 0).toDouble();
                    double u = getQStringListIndexValue(_linelist, 1).toDouble();

                    property->_luCurve.push_back({ load,u });
                    currentline = _file.readLine().simplified();
                }
            }
            else
            {
                property->_nonlinear = "线性";
                property->_value = currentline;
                currentline = _file.readLine().simplified();
            }

        }
        else if (!type.compare("SPRINGA", Qt::CaseInsensitive))
        {
            MechPropertyData::mSpring* property = new MechPropertyData::mSpring(phyID, SpringAProperty, propertyname);
            _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);

            currentline = _file.readLine().simplified();
            if (!NONLINEAR.compare("on", Qt::CaseInsensitive))
            {
                property->_nonlinear = "非线性";
                while (!_file.atEnd())
                {
                    if (currentline.startsWith("*")) break;

                    _linelist = currentline.split(",");
                    double load = getQStringListIndexValue(_linelist, 0).toDouble();
                    double u = getQStringListIndexValue(_linelist, 1).toDouble();

                    property->_luCurve.push_back({ load,u });
                    currentline = _file.readLine().simplified();
                }
            }
            else
            {
                property->_nonlinear = "线性";
                property->_value = currentline;
                currentline = _file.readLine().simplified();
            }

        }
    }
    void mIMxFile::readViscousdampsection(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mViscousdampsection* property = new MechPropertyData::mViscousdampsection(phyID, ViscousDampProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        currentline = _file.readLine().simplified();
        property->_GE = currentline;
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readBushsection(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mCbush* property = new MechPropertyData::mCbush(phyID, CbushProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);

        auto setCbushValue = [property, this](int index) {
            property->_isCheck[index] = true;
            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            for (int i = 0; i < 6; i++)
            {
                property->_value[index][i] = getQStringListIndexValue(_linelist, i).toDouble();
            }
            };

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*")) break;
            else if (!currentline.compare("k", Qt::CaseInsensitive)) setCbushValue(0);
            else if (!currentline.compare("km", Qt::CaseInsensitive)) setCbushValue(1);
            else if (!currentline.compare("b", Qt::CaseInsensitive)) setCbushValue(2);
            else if (!currentline.compare("ge", Qt::CaseInsensitive)) setCbushValue(3);
            else if (!currentline.compare("ang", Qt::CaseInsensitive)) setCbushValue(4);
            else if (!currentline.compare("m", Qt::CaseInsensitive)) setCbushValue(5);

            currentline = _file.readLine().simplified();
        }
    }
    void mIMxFile::readPlanestrainsection(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mShellHomo* property = new MechPropertyData::mShellHomo(phyID, PlaneStrainProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        currentline = _file.readLine().simplified();
        property->_thickness = currentline;
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readPlanestresssection(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mShellHomo* property = new MechPropertyData::mShellHomo(phyID, PlaneStressProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        currentline = _file.readLine().simplified();
        property->_thickness = currentline;
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readGasketsection(const int& phyID, const QString& propertyname)
    {
        MechPropertyData::mGasket* property = new MechPropertyData::mGasket(phyID, GasketProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        property->_initialThickness = getQStringListIndexValue(_linelist, 0);
        property->_initialGap = getQStringListIndexValue(_linelist, 1);
        if (property->_initialThickness.isEmpty() || property->_initialThickness.toDouble() == 0)
        {
            property->_thicknessDef = "自动计算";
        }
        else
        {
            property->_thicknessDef = "指定厚度";
        }
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readGapunisection(const int& phyID, const QString& propertyname)
    {
        QString type = getKeyValue(currentline, "type", "=");
        if (!type.compare("gapuni", Qt::CaseInsensitive))
        {
            MechPropertyData::mGapuni* property = new MechPropertyData::mGapuni(phyID, GapuniProperty, propertyname);
            _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            property->_gapDistance = getQStringListIndexValue(_linelist, 0);
            QString x = getQStringListIndexValue(_linelist, 1);
            QString y = getQStringListIndexValue(_linelist, 2);
            QString z = getQStringListIndexValue(_linelist, 3);
            if (!(x.isEmpty() && y.isEmpty() && z.isEmpty()))
            {
                property->_direction = QString("%1,%2,%3").arg(x.toDouble())
                    .arg(y.toDouble()).arg(z.toDouble());
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            currentline = _file.readLine().simplified();
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("截面属性 %1  类型不支持！").arg(propertyname));
        }
    }
    void mIMxFile::readMultiscalesection(const int& phyID, const QString& propertyname)
    {
        if (currentline.startsWith("*fiberreinforce", Qt::CaseInsensitive))
        {
            MechPropertyData::mMacroEquivalent* property = new MechPropertyData::mMacroEquivalent(phyID, MacroEquivalentSection, propertyname);
            _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);
            QString rveName = getKeyValue(currentline, "rve", "=");
            QFileInfo info(rveName);
            QString pureName = info.baseName();
            property->_frbName = pureName;
            MechIOFile::mIOFileManager::getInstance()->slot_importMfFile(rveName);
            currentline = _file.readLine().simplified();
        }
    }
    void mIMxFile::readPlyLaminate(const int& phyID, const QString& propertyname)
    {
        //1. 读取 sys="坐标系名"
        QString sysName = getKeyValue(currentline, "sys", "=");
        QString element = getKeyValue(currentline, "element", "=");

        MechPropertyData::mPlyLaminate* property = new MechPropertyData::mPlyLaminate(phyID, PlyLaminateProperty, propertyname);
        _modeldata->addPhysicalField<MechPropertyData::mPropertyBase>(property);

        //2. 有坐标系：建立映射 + 赋值
        if (!sysName.isEmpty())
        {
            appendMapPhysicalStruct(Physical_Property, property->getPropertyId(), Physical_Coordinate, sysName);
            MechCoordinateData::mCoordinateBase* coor = _modeldata->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(sysName);
            if (coor)
            {
                property->_normaldirection = coor->_normaldirection;
                property->_rotAngleType = coor->_rotAngleType;
                property->_rotAngle = coor->_rotAngle;
                appendMapPhysicalStruct(Physical_Property, property->getPropertyId(), Physical_Field, coor->_rotFieldName, MapRelation_RotationField);
            }
        }

        // 按单元类型分发解析
        if (element == "continuum-shell")
        {
            property->_type = 1;
            readSolidShell(property);
        }
        else if (element == "solid")
        {
            property->_type = 2;
            readSolid(property);
        }
        else
        {
            property->_type = 0;
            readShell(property);
        }
    }
    void mIMxFile::readSolidShell(MechPropertyData::mPlyLaminate* prodata)
    {
        if (!prodata) return;

        // 读取下一行，交给对应类型函数解析
        currentline = _file.readLine().simplified();

        while (!_file.atEnd())
        {
            if (currentline.startsWith("*"))
            {
                if (!currentline.startsWith("*ply", Qt::CaseInsensitive) &&
                    !currentline.startsWith("*laminationoption", Qt::CaseInsensitive))
                    break;
            }
            else if (currentline.isEmpty())
            {
                currentline = _file.readLine().simplified();
                continue;
            }

            if (currentline.startsWith("*ply", Qt::CaseInsensitive))
            {
                QString plyType = getKeyValue(currentline, "type", "=");
                if (plyType == "define")
                {
                    prodata->_plyDefType = 0;
                    currentline = _file.readLine().simplified();
                    std::vector<QString> nameList;
                    while (!_file.atEnd())
                    {
                        if (currentline.startsWith("*")) break;
                        else if (currentline.isEmpty())
                        {
                            currentline = _file.readLine().simplified();
                            continue;
                        }
                        _linelist = currentline.split(",");
                        QString matName = getQStringListIndexValue(_linelist, 0);
                        nameList.push_back(matName);
                        QString angle = getQStringListIndexValue(_linelist, 1);
                        QString thick = getQStringListIndexValue(_linelist, 2);
                        QString point = getQStringListIndexValue(_linelist, 3);
                        prodata->thicknessList.append(thick);
                        prodata->angleList.append(angle);
                        prodata->intePointList.append(point);
                        currentline = _file.readLine().simplified();
                    }
                    appendMapPhysicalTable(Physical_Property, prodata->getPropertyId(), Physical_Material, nameList);
                }
                else if (plyType == "laminate")
                {
                    prodata->_plyDefType = 1;
                    QString lamName = getKeyValue(currentline, "laminate", "=");
                    appendMapPhysicalStruct(Physical_Property, prodata->getPropertyId(), Physical_Laminate, lamName);
                    currentline = _file.readLine().simplified();
                }
                else
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("*ply 缺少 type 属性或类型不支持！"));
                    currentline = _file.readLine().simplified();
                    while (!_file.atEnd())
                    {
                        if (currentline.startsWith("*"))
                            break;
                        else if (currentline.isEmpty())
                        {
                            currentline = _file.readLine().simplified();
                            continue;
                        }
                        currentline = _file.readLine().simplified();
                    }
                }
            }
            else if (currentline.startsWith("*laminationoption", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                if (currentline == "sym")
                    prodata->_solidShellProperty = "对称";
                currentline = _file.readLine().simplified();
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::readSolid(MechPropertyData::mPlyLaminate* prodata)
    {
        if (!prodata) return;
        // 先读取堆叠方向（该行属于section行参数）
        QString stack = getKeyValue(currentline, "stackdirection", "=");
        prodata->eleMentDir = stack.toInt();
        currentline = _file.readLine().simplified();

        while (!_file.atEnd())
        {
            if (currentline.startsWith("*"))
            {
                if (!currentline.startsWith("*ply", Qt::CaseInsensitive) &&
                    !currentline.startsWith("*laminationoption", Qt::CaseInsensitive))
                    break;
            }
            else if (currentline.isEmpty())
            {
                currentline = _file.readLine().simplified();
                continue;
            }

            if (currentline.startsWith("*ply", Qt::CaseInsensitive))
            {
                QString plyType = getKeyValue(currentline, "type", "=");
                if (plyType == "define")
                {
                    prodata->_plyDefType = 0;
                    currentline = _file.readLine().simplified();
                    std::vector<QString> nameList;
                    while (!_file.atEnd())
                    {
                        if (currentline.startsWith("*")) break;
                        else if (currentline.isEmpty())
                        {
                            currentline = _file.readLine().simplified();
                            continue;
                        }
                        _linelist = currentline.split(",");
                        QString matName = getQStringListIndexValue(_linelist, 0);
                        nameList.push_back(matName);
                        QString angle = getQStringListIndexValue(_linelist, 1);
                        QString thick = getQStringListIndexValue(_linelist, 2);
                        QString point = getQStringListIndexValue(_linelist, 3);
                        prodata->thicknessList.append(thick);
                        prodata->angleList.append(angle);
                        prodata->intePointList.append(point);
                        currentline = _file.readLine().simplified();
                    }
                    appendMapPhysicalTable(Physical_Property, prodata->getPropertyId(), Physical_Material, nameList);
                }
                else if (plyType == "laminate")
                {
                    prodata->_plyDefType = 1;
                    QString lamName = getKeyValue(currentline, "laminate", "=");
                    appendMapPhysicalStruct(Physical_Property, prodata->getPropertyId(), Physical_Laminate, lamName);
                    currentline = _file.readLine().simplified();
                }
                else
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("*ply 缺少 type 属性或类型不支持！"));
                    currentline = _file.readLine().simplified();
                    while (!_file.atEnd())
                    {
                        if (currentline.startsWith("*"))
                            break;
                        else if (currentline.isEmpty())
                        {
                            currentline = _file.readLine().simplified();
                            continue;
                        }
                        currentline = _file.readLine().simplified();
                    }
                }
            }
            else if (currentline.startsWith("*laminationoption", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                if (currentline == "sym")
                    prodata->_solidProperty = "对称";
                currentline = _file.readLine().simplified();
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::readShell(MechPropertyData::mPlyLaminate* prodata)
    {
        if (!prodata) return;

        // 读取下一行，交给对应类型函数解析
        currentline = _file.readLine().simplified();

        while (!_file.atEnd())
        {
            if (currentline.startsWith("*"))
            {
                if (!currentline.startsWith("*ply", Qt::CaseInsensitive) &&
                    !currentline.startsWith("*laminationoption", Qt::CaseInsensitive) &&
                    !currentline.startsWith("*referenceplane", Qt::CaseInsensitive) &&
                    !currentline.startsWith("*failtheory", Qt::CaseInsensitive))
                    break;
            }
            else if (currentline.isEmpty())
            {
                currentline = _file.readLine().simplified();
                continue;
            }

            if (currentline.startsWith("*ply", Qt::CaseInsensitive)) //读铺层定义
            {
                QString plyType = getKeyValue(currentline, "type", "=");
                // 支持厚度离散场读取：*ply, type = ..., thickfield = "xxx"
                QString thickFieldName = getKeyValue(currentline, "thickfield", "=");
                if (!thickFieldName.isEmpty())
                {
                    appendMapPhysicalStruct(Physical_Property, prodata->getPropertyId(), Physical_Field, thickFieldName, MapRelation_ThickField);
                    _tempPlyThickFieldNames[prodata->getPropertyId()] = thickFieldName;
                }
                if (plyType == "define") //直接定义
                {
                    prodata->_plyDefType = 0;
                    currentline = _file.readLine().simplified();
                    std::vector<QString> nameList;
                    while (!_file.atEnd())
                    {
                        if (currentline.startsWith("*"))
                            break;
                        else if (currentline.isEmpty())
                        {
                            currentline = _file.readLine().simplified();
                            continue;
                        }
                        _linelist = currentline.split(",");
                        QString matName = getQStringListIndexValue(_linelist, 0);
                        nameList.push_back(matName);
                        QString angle = getQStringListIndexValue(_linelist, 1);
                        QString thick = getQStringListIndexValue(_linelist, 2);
                        QString point = getQStringListIndexValue(_linelist, 3);

                        prodata->thicknessList.append(thick);
                        prodata->angleList.append(angle);
                        prodata->intePointList.append(point);
                        currentline = _file.readLine().simplified();
                    }
                    appendMapPhysicalTable(Physical_Property, prodata->getPropertyId(), Physical_Material, nameList);
                }
                else if (plyType == "laminate") //层合板引用
                {
                    prodata->_plyDefType = 1;
                    QString lamName = getKeyValue(currentline, "laminate", "=");
                    appendMapPhysicalStruct(Physical_Property, prodata->getPropertyId(), Physical_Laminate, lamName);
                    currentline = _file.readLine().simplified();
                }
                else
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("*ply 缺少 type 属性或类型不支持！"));
                    currentline = _file.readLine().simplified();
                    while (!_file.atEnd())
                    {
                        if (currentline.startsWith("*"))
                            break;
                        else if (currentline.isEmpty())
                        {
                            currentline = _file.readLine().simplified();
                            continue;
                        }
                        currentline = _file.readLine().simplified();
                    }
                }
            }
            else if (currentline.startsWith("*laminationoption", Qt::CaseInsensitive)) //铺层选项
            {
                currentline = _file.readLine().simplified();
                if (currentline == "sym")          prodata->_shellProperty = "对称铺层";
                else if (currentline == "bend")   prodata->_shellProperty = "弯曲行为";
                else if (currentline == "mem")    prodata->_shellProperty = "膜行为";
                else if (currentline == "smear")  prodata->_shellProperty = "等效单层";
                else if (currentline == "sybend") prodata->_shellProperty = "对称弯曲";
                else if (currentline == "symem") prodata->_shellProperty = "对称膜";
                else if (currentline == "sysmear")prodata->_shellProperty = "对称等效单层";
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*referenceplane", Qt::CaseInsensitive))//参考面
            {
                QString type = getKeyValue(currentline, "type", "=");
                if (type == "zoffs")
                {
                    prodata->_referenceSurfacePos = "偏置距离";
                    currentline = _file.readLine().simplified();
                    prodata->_referenceSurfaceDis = currentline;
                }
                else if (type == "ratio")
                {
                    prodata->_referenceSurfacePos = "偏置率";
                    currentline = _file.readLine().simplified();
                    prodata->_referenceSurfaceDis = currentline;
                }
                else if (type == "top")
                    prodata->_referenceSurfacePos = "顶部";
                else if (type == "bottom")
                    prodata->_referenceSurfacePos = "底部";
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*failtheory", Qt::CaseInsensitive))//失效准则
            {
                currentline = _file.readLine().simplified();
                if (currentline == "max_stress")     prodata->_failure = "最大应力";
                else if (currentline == "max_strain")prodata->_failure = "最大应变";
                else if (currentline == "tsai_hill") prodata->_failure = "Tsai-Hill";
                else if (currentline == "tsai_wu")  prodata->_failure = "Tsai-Wu";
                else if (currentline == "azzi_tasi_hill") prodata->_failure = "Azzi-Tsai-Hill";
                else if (currentline == "hoff")     prodata->_failure = "Hoff";
                else if (currentline == "puck")     prodata->_failure = "Puck";
                else if (currentline == "hfail")     prodata->_failure = "Hashin";
                currentline = _file.readLine().simplified();
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }
    }
    int mIMxFile::createSectionShape(const QString& type, vector<double>& xbs, bool isPeami)
    {
        int id = _modeldata->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).createID();
        MechSectionShapeData::mSectionShapeBase* shapeBase = nullptr;
        QString name; int size = 0;
        QList<double> _para;

        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        if (!type.compare("h", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::H_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "H"), id);
        }
        else if (!type.compare("i", Qt::CaseInsensitive))
        {
            size = 6;
            shapeBase = new MechSectionShapeData::I_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "I"), id);
        }
        else if (!type.compare("l", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::L_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "L"), id);
        }
        else if (!type.compare("t", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::T_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "T"), id);
        }
        else if (!type.compare("box", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::Box_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Box"), id);
        }
        else if (!type.compare("rectangular", Qt::CaseInsensitive))
        {
            size = 2;
            shapeBase = new MechSectionShapeData::Rectangle_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Rec"), id);
        }
        else if (!type.compare("circular", Qt::CaseInsensitive))
        {
            size = 1;
            shapeBase = new MechSectionShapeData::Circle_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Cir"), id);
        }
        else if (!type.compare("ring", Qt::CaseInsensitive))
        {
            size = 2;
            shapeBase = new MechSectionShapeData::Annular_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Ann"), id);
        }
        else if (!type.compare("chan", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::Channel_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Channel"), id);
        }
        else if (!type.compare("chan1", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::Channel1_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Channel1"), id);
        }
        else if (!type.compare("chan2", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::Channel2_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Channel2"), id);
        }
        else if (!type.compare("i1", Qt::CaseInsensitive))
        {
            size = 6;
            shapeBase = new MechSectionShapeData::I1_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "I1"), id);
        }
        else if (!type.compare("t1", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::T1_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "T1"), id);
        }
        else if (!type.compare("t2", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::T2_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "T2"), id);
        }
        else if (!type.compare("box1", Qt::CaseInsensitive))
        {
            size = 6;
            shapeBase = new MechSectionShapeData::Box1_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Box1"), id);
        }
        else if (!type.compare("cross", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::Cross_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Cross"), id);
        }
        else if (!type.compare("hat", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::Hat_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Hat"), id);
        }
        else if (!type.compare("z", Qt::CaseInsensitive))
        {
            size = 4;
            shapeBase = new MechSectionShapeData::Z_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Z"), id);
        }
        //自定义截面
        else
        {
            auto customSec = new MechSectionShapeData::mCustomSectionShape(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Custom"), id);
            getCVM->getModelData()->addPhysicalField<MechSectionShapeData::mSectionShapeBase>(customSec);
            customSec->CreateSec();

            QList<QString> list_para = customSec->getCustomPara();
            for (int i = 0; i < _linelist.size(); i++)
            {
                list_para[i] = getQStringListIndexValue(_linelist, i);
            }

            currentline = _file.readLine().simplified();
            if (!currentline.startsWith("*"))
            {
                _linelist = currentline.split(",");
                for (int i = 0; i < _linelist.size(); i++)
                {
                    list_para[i + 16] = getQStringListIndexValue(_linelist, i);
                }
            }

            customSec->setCustomPara(list_para);
            return id;
        }

        if (shapeBase == nullptr)
            return -1;

        getCVM->getModelData()->addPhysicalField<MechSectionShapeData::mSectionShapeBase>(shapeBase);

        int index = 0;
        if (isPeami)
        {
            index = 1;
            xbs.push_back(getQStringListIndexValue(_linelist, 0).toDouble());
        }
        else
        {
            xbs.push_back(0);
        }
        double value = 0.0;
        for (int i = 0; i < size; i++)
        {
            value = getQStringListIndexValue(_linelist, i + index).toDouble();
            if ((!type.compare("i1", Qt::CaseInsensitive) && i == 0)
                || (!type.compare("h", Qt::CaseInsensitive) && i == 1)
                || (!type.compare("cross", Qt::CaseInsensitive) && i == 0)) {
                value *= 0.5;
            }
            _para.push_back(value);
        }
        shapeBase->setPara(_para);

        if (_linelist.size() > size + index)
            shapeBase->setNsm(getQStringListIndexValue(_linelist, size + index));

        shapeBase->CreateSec();
        shapeBase->clearData();
        return id;
    }
    void mIMxFile::readSetData(vector<RenderPickResultData>& ids, QString generate, int maxID, bool isPart,const int setID)
    {
        int partID = -1;
        int phyID;
        if (!generate.compare("on", Qt::CaseInsensitive))
        {
            int firstId, lastId, increment;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                //判断终止条件
                if (currentline.contains("*"))
                {
                    break;
                }
                else if (currentline.isEmpty())
                {
                    continue;
                }

                if (_linelist.size() >= 2)
                {
                    firstId = _linelist[0].toInt();
                    lastId = _linelist[1].toInt();
                    increment = 1;
                    if (_linelist.size() > 2)
                        increment = _linelist[2].toInt();

                    for (; firstId <= lastId; firstId += increment)
                    {
                        phyID = firstId + maxID;
                        if (isPart && _elementIDToPartID.contains(phyID))
                            partID = _elementIDToPartID[phyID];
                        appendRenderPickResultData(ids, phyID, partID);
                    }
                }
            }
        }
        else
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("\""))
            {
                QString setName;
                while (!_file.atEnd())
                {
                    _linelist = currentline.split(",");
                    //判断终止条件
                    if (currentline.contains("*"))
                    {
                        break;
                    }
                    else if (currentline.isEmpty())
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }

                    for (int i = 0; i < _linelist.size(); i++)
                    {
                        QString pureName = getQStringListIndexValue(_linelist, i);
                        if (pureName.isEmpty())
                            continue;
                        m_NodesurfaceHash[setID].push_back(pureName);
                    }
                    currentline = _file.readLine().simplified();
                }
            }
            else
            {

                while (!_file.atEnd())
                {
                    _linelist = currentline.split(",");
                    //判断终止条件
                    if (currentline.contains("*"))
                    {
                        break;
                    }
                    else if (currentline.isEmpty())
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }
                    for (auto& id : _linelist)
                    {
                        if (id.isEmpty())
                            continue;
                        phyID = id.simplified().toInt() + maxID;
                        if (isPart && _elementIDToPartID.contains(phyID))
                            partID = _elementIDToPartID[phyID];
                        appendRenderPickResultData(ids, phyID, partID);
                    }

                    currentline = _file.readLine().simplified();
                }
            }
        }
    }

    void mIMxFile::readDofSetData(vector<RenderPickResultData>& ids, QString generate, int maxID, bool isPart)
    {
        QString dofstr = "";
        int partID = -1;
        int phyID;

        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*"))
                break;

            _linelist = currentline.split(",");
            dofstr = _linelist[1];
            QString id = _linelist[0];
            if (id.isEmpty())
                continue;
            phyID = id.simplified().toInt() + maxID;
            if (isPart && _elementIDToPartID.contains(phyID))
                partID = _elementIDToPartID[phyID];
            appendRenderPickResultData(ids, phyID, partID);

        }

    }


    void mIMxFile::readSurface(vector<RenderPickResultData>& ids, QString generate, int setID)
    {
        int phyID, partID, index;
        QString bnumber;
        QString setName;

        auto getIndexByBnumber = [&]() {
            if (!bnumber.compare("BPOS", Qt::CaseInsensitive))
            {
                index = 0;
            }
            else if (!bnumber.compare("BNEG", Qt::CaseInsensitive))
            {
                index = -1;
            }
            else
            {
                index = bnumber.split("B").last().simplified().toInt();//单元的第几面
            }
            };

        if (!generate.compare("on", Qt::CaseInsensitive))
        {
            int firstId, lastId, increment;
            QString incrementStr;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                //判断终止条件
                if (currentline.contains("*"))
                {
                    break;
                }
                else if (currentline.isEmpty())
                {
                    continue;
                }

                if (_linelist.size() >= 4)
                {
                    firstId = getQStringListIndexValue(_linelist, 0).toInt();
                    lastId = getQStringListIndexValue(_linelist, 1).toInt();
                    incrementStr = getQStringListIndexValue(_linelist, 2);
                    if (incrementStr.isEmpty()) increment = 1;
                    else increment = incrementStr.toInt();
                    bnumber = getQStringListIndexValue(_linelist, 3);
                    getIndexByBnumber();

                    for (; firstId <= lastId; firstId += increment)
                    {
                        phyID = firstId + _lastMaxMeshID;
                        if (_elementIDToPartID.contains(phyID))
                            partID = _elementIDToPartID[phyID];
                        else
                            continue;

                        appendRenderPickResultData(ids, phyID, partID, index);
                    }
                }
            }
        }
        else
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("\""))
            {
                while (!_file.atEnd())
                {
                    _linelist = currentline.split(",");
                    //判断终止条件
                    if (currentline.contains("*"))
                    {
                        break;
                    }
                    else if (currentline.isEmpty())
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }
                    currentline = _file.readLine().simplified();
                    setName = getQStringListIndexValue(_linelist, 0);
                    if (setName.isEmpty())
                        continue;
                    setName = ExistRenamePhysicalName(Physical_Set, setName);
                    bnumber = getQStringListIndexValue(_linelist, 1);
                    getIndexByBnumber();

                    _elementSurfaceToElement[setID][setName] = index;
                }
            }
            else
            {
                while (!_file.atEnd())
                {
                    _linelist = currentline.split(",");
                    //判断终止条件
                    if (currentline.contains("*"))
                    {
                        break;
                    }
                    else if (currentline.isEmpty())
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }

                    currentline = _file.readLine().simplified();
                    phyID = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxMeshID;
                    bnumber = getQStringListIndexValue(_linelist, 1);
                    getIndexByBnumber();

                    if (_elementIDToPartID.contains(phyID))
                        partID = _elementIDToPartID[phyID];
                    else
                        continue;

                    appendRenderPickResultData(ids, phyID, partID, index);
                }
            }
        }
    }
    void mIMxFile::readEdge(vector<RenderPickResultData>& ids, QString generate, int setID)
    {
        int phyID, partID, index;
        QString bnumber;
        QString setName;
        QString incrementStr;

        map<int, std::vector<std::pair<int, int>>> meshIdToBnumber;
        if (!generate.compare("on", Qt::CaseInsensitive))
        {
            int firstId, lastId, increment;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                //判断终止条件
                if (currentline.contains("*"))
                {
                    break;
                }
                else if (currentline.isEmpty())
                {
                    continue;
                }

                if (_linelist.size() >= 4)
                {
                    firstId = getQStringListIndexValue(_linelist, 0).toInt();
                    lastId = getQStringListIndexValue(_linelist, 1).toInt();
                    incrementStr = getQStringListIndexValue(_linelist, 2).toInt();
                    if (incrementStr.isEmpty()) increment = 1;
                    else increment = incrementStr.toInt();
                    bnumber = getQStringListIndexValue(_linelist, 3);
                    index = bnumber.split("B").last().simplified().toInt();

                    for (; firstId <= lastId; firstId += increment)
                    {
                        phyID = firstId + _lastMaxMeshID;
                        if (_elementIDToPartID.contains(phyID))
                            partID = _elementIDToPartID[phyID];
                        else
                            continue;

                        meshIdToBnumber[partID].push_back({ phyID,index });
                    }
                }
            }
        }
        else
        {
            currentline = _file.readLine().simplified();
            if (currentline.contains("\""))
            {
                while (!_file.atEnd())
                {
                    _linelist = currentline.split(",");
                    //判断终止条件
                    if (currentline.contains("*"))
                    {
                        break;
                    }
                    else if (currentline.isEmpty())
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }
                    currentline = _file.readLine().simplified();
                    setName = getQStringListIndexValue(_linelist, 0);
                    if (setName.isEmpty())
                        continue;
                    setName = ExistRenamePhysicalName(Physical_Set, setName);
                    bnumber = getQStringListIndexValue(_linelist, 1);
                    index = bnumber.split("B").last().simplified().toInt();
                    _elementSurfaceToElement[setID][setName] = index;
                }
            }
            else
            {
                while (!_file.atEnd())
                {
                    _linelist = currentline.split(",");
                    //判断终止条件
                    if (currentline.contains("*"))
                    {
                        break;
                    }
                    else if (currentline.isEmpty())
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }

                    currentline = _file.readLine().simplified();
                    phyID = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxMeshID;
                    bnumber = getQStringListIndexValue(_linelist, 1);
                    index = bnumber.split("B").last().simplified().toInt();

                    if (_elementIDToPartID.contains(phyID))
                        partID = _elementIDToPartID[phyID];
                    else
                        continue;

                    meshIdToBnumber[partID].push_back({ phyID,index });
                }
            }
        }

        if (meshIdToBnumber.size() > 0)
        {
            ids = _meshData->getEdgeNodeByPartElementBN(meshIdToBnumber);
        }
    }
    void mIMxFile::transformMixSet(int setID)
    {
        MechSetData::mSetBase* setBase = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
        if (setBase == nullptr || !setBase->getInternal() || setBase->getSetType() == Mix)
            return;

        MechSetData::mMixSet* mix = new MechSetData::mMixSet(setID, Mix, setBase->getSetName(), setBase->getInternal(), setBase->getIsUserRename());
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> data;
        switch (setBase->getSetType())
        {
        case Node:
        {
            MechSetData::mNode* node = dynamic_cast<MechSetData::mNode*>(setBase);
            data[MxFunctions::PickDataType::Node] = node->getNodeData();
            break;
        }
        case Element:
        {
            MechSetData::mElementSet* element = dynamic_cast<MechSetData::mElementSet*>(setBase);
            data[MxFunctions::PickDataType::Element] = element->getElementSetData();
            break;
        }
        case ElementSurface:
        {
            MechSetData::mElementSurface* surface = dynamic_cast<MechSetData::mElementSurface*>(setBase);
            data[MxFunctions::PickDataType::ElementFace] = surface->getElementSurfaceData();
            break;
        }
        case ElementEdge:
        {
            MechSetData::mElementSurface* surface = dynamic_cast<MechSetData::mElementSurface*>(setBase);
            data[MxFunctions::PickDataType::ElementEdge] = surface->getElementSurfaceData();
            break;
        }
        default:
            break;
        }

        mix->setMixSetData(data);
        _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).replaceSetData(mix);
    }
    void mIMxFile::appendRenderPickResultData(vector<RenderPickResultData>& ids, const int& result_id, int partID, int result_id2)
    {
        RenderPickResultData data;
        data.result_id = result_id;
        data.part_id = partID;
        data.result_id2 = result_id2;
        ids.push_back(data);
    }
    //读后统一回填点面/线面接触 slave_region：从面为节点集合/节点面集（数据为节点）时置 1（节点表面），
    //其余保持 0（单元面）。集合此时已全部读入且映射已解析，判定与卡片先后顺序无关
    void mIMxFile::rebuildNodeToSurfaceSlaveRegion()
    {
        auto& contactManager = _modeldata->getManager<MechContactData::mContactManager>(Physical_Contact);
        QVector<int> contactIds = QVector<int>::fromStdVector(
            contactManager.getAllContactIdsByTypes(MxFunctions::NodeToSurfaceContact));
        contactIds.append(QVector<int>::fromStdVector(
            contactManager.getAllContactIdsByTypes(MxFunctions::LineToSurfaceContact)));
        for (int contactId : contactIds)
        {
            MechContactData::mContactBase* contact = contactManager.getContactData(contactId);
            if (!contact)
                continue;

            const int setId = _modeldata->getMapRelationManager().getMapRelation<int>(
                { Physical_Contact, contactId }, Physical_Set, MapRelation_Slave);
            if (setId == -1)
                continue;
            MechSetData::mSetBase* set = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setId);
            if (!set)
                continue;

            if (set->getSetType() == MxFunctions::Node || set->getSetType() == MxFunctions::NodeSurface)
            {
                contact->slave_region = 1;
            }
        }
    }

    void mIMxFile::readNodeToSurfaceContact(MechContactData::mContactBase* contact)
    {
        contact->setContactType(!getKeyValue(currentline, "line", "=").compare("on", Qt::CaseInsensitive)
            ? LineToSurfaceContact : NodeToSurfaceContact);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        QString slaverName = getQStringListIndexValue(_linelist, 0);
        slaverName = ExistRenamePhysicalName(Physical_Set, slaverName);
        QString masterName = getQStringListIndexValue(_linelist, 1);
        masterName = ExistRenamePhysicalName(Physical_Set, masterName);

        // 从面为节点集合时回填区域标记：仅覆盖先于接触卡读入的集合（如接触段内联写出的
        // 内部节点集合）；用户自建集合卡在接触卡之后，统一由 rebuildNodeToSurfaceSlaveRegion
        // 在映射解析完成后按集合真实类型回填
        if (_nodeSetNames.contains(slaverName))
        {
            contact->slave_region = 1;
        }

        appendMapPhysicalStruct(Physical_Contact, contact->getContactId(), Physical_Set, slaverName, MapRelation_Slave);
        appendMapPhysicalStruct(Physical_Contact, contact->getContactId(), Physical_Set, masterName, MapRelation_Master);
        _contact_hash[{ slaverName, masterName }] = contact->getContactId();

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*behavior", Qt::CaseInsensitive))
            {
                readBehavior(contact);
            }
            else if (currentline.startsWith("*friction", Qt::CaseInsensitive))
            {
                readFriction(contact);
            }
            else if (currentline.startsWith("*gapconductance", Qt::CaseInsensitive))
            {
                readGapconductance(contact);
            }
            else if (currentline.startsWith("*adjust", Qt::CaseInsensitive))
            {
                readAdjust(contact);
            }
            else
            {
                break;
            }
        }
    }
    void mIMxFile::readSurfaceToSurfaceContact(MechContactData::mContactBase* contact)
    {
        const bool is2D = !getKeyValue(currentline, "line", "=").compare("on", Qt::CaseInsensitive);
        contact->setContactType(is2D ? TwoDimensionalContact : SurfaceToSurfaceContact);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        QString slaverName = getQStringListIndexValue(_linelist, 0);
        slaverName = ExistRenamePhysicalName(Physical_Set, slaverName);
        QString masterName = getQStringListIndexValue(_linelist, 1);
        masterName = ExistRenamePhysicalName(Physical_Set, masterName);
        appendMapPhysicalStruct(Physical_Contact, contact->getContactId(), Physical_Set, slaverName, MapRelation_Slave);
        appendMapPhysicalStruct(Physical_Contact, contact->getContactId(), Physical_Set, masterName, MapRelation_Master);
        _contact_hash[{ slaverName, masterName }] = contact->getContactId();

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*behavior", Qt::CaseInsensitive))
            {
                readBehavior(contact);
            }
            else if (currentline.startsWith("*friction", Qt::CaseInsensitive))
            {
                readFriction(contact);
            }
            else if (currentline.startsWith("*gapconductance", Qt::CaseInsensitive))
            {
                readGapconductance(contact);
            }
            else if (currentline.startsWith("*adjust", Qt::CaseInsensitive))
            {
                readAdjust(contact);
            }
            else if (currentline.startsWith("*clearance", Qt::CaseInsensitive))
            {
                readClearance(contact);
            }
            else if (currentline.startsWith("*evaluation", Qt::CaseInsensitive))
            {
                readEvaluation(contact);
            }
            else
            {
                break;
            }
        }
    }
    void mIMxFile::readCommonContact(MechContactData::mContactBase* contact)
    {
        contact->setContactType(CommonContact);
        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*behavior", Qt::CaseInsensitive))
            {
                readBehavior(contact);
            }
            else if (currentline.startsWith("*friction", Qt::CaseInsensitive))
            {
                readFriction(contact);
            }
            else if (currentline.startsWith("*gapconductance", Qt::CaseInsensitive))
            {
                readGapconductance(contact);
            }
            else if (currentline.startsWith("*adjust", Qt::CaseInsensitive))
            {
                readAdjust(contact);
            }
            else if (currentline.startsWith("*clearance", Qt::CaseInsensitive))
            {
                readClearance(contact);
            }
            else
            {
                break;
            }
        }
    }
    void mIMxFile::readSelfContact(MechContactData::mContactBase* contact)
    {
        contact->setContactType(SelfContact);
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        QString masterName = getQStringListIndexValue(_linelist, 0);
        masterName = ExistRenamePhysicalName(Physical_Set, masterName);
        appendMapPhysicalStruct(Physical_Contact, contact->getContactId(), Physical_Set, masterName, MapRelation_Master);

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*behavior", Qt::CaseInsensitive))
            {
                readBehavior(contact);
            }
            else if (currentline.startsWith("*friction", Qt::CaseInsensitive))
            {
                readFriction(contact);
            }
            else if (currentline.startsWith("*gapconductance", Qt::CaseInsensitive))
            {
                readGapconductance(contact);
            }
            else if (currentline.startsWith("*adjust", Qt::CaseInsensitive))
            {
                readAdjust(contact);
            }
            else if (currentline.startsWith("*clearance", Qt::CaseInsensitive))
            {
                readClearance(contact);
            }
            else
            {
                break;
            }
        }
    }
    void mIMxFile::readBehavior(MechContactData::mContactBase* contact)
    {
        QString pressurecurve = getKeyValue(currentline, "pressurecurve", "=");
        QString type = getKeyValue(currentline, "type", "=");
        QString scalefactor = getKeyValue(currentline, "scalefactor", "=");
        if (!pressurecurve.compare("hard", Qt::CaseInsensitive))
        {
            contact->overclosure = "硬接触";
        }
        else if (!pressurecurve.compare("tie", Qt::CaseInsensitive))
        {
            contact->overclosure = "完全绑定接触";
        }
        else if (!pressurecurve.compare("linear", Qt::CaseInsensitive))
        {
            contact->overclosure = "线性接触";
        }
        else if (!pressurecurve.compare("normaltie", Qt::CaseInsensitive))
        {
            contact->overclosure = "法向不分离接触";
        }

        if (!type.compare("penalty", Qt::CaseInsensitive))
        {
            contact->constraint = "罚函数法";
        }
        else if (!type.compare("mpc", Qt::CaseInsensitive))
        {
            contact->constraint = "多点约束法";
        }
        else if (!type.compare("augment", Qt::CaseInsensitive))
        {
            contact->constraint = "增广-摄动拉格朗日乘子法";
        }

        if (!scalefactor.isEmpty())
        {
            contact->scale_factor = scalefactor;
        }

        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*")) return;
        contact->penalty_factor = currentline;
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readFriction(MechContactData::mContactBase* contact)
    {
        contact->tangential_define = true;
        QString type = getKeyValue(currentline, "type", "=");
        QString sliptolerance = getKeyValue(currentline, "sliptolerance", "=");
        QString elasticslip = getKeyValue(currentline, "elasticslip", "=");

        if (!type.compare("general", Qt::CaseInsensitive))
        {
            contact->tangential_friction = "通用接触摩擦";
            currentline = _file.readLine().simplified();
            contact->tangential_static_coef = currentline.toDouble();
        }
        else if (!type.compare("frictionless", Qt::CaseInsensitive))
        {
            contact->tangential_friction = "无摩擦";
        }
        else if (!type.compare("rough", Qt::CaseInsensitive))
        {
            contact->tangential_friction = "粗糙";
        }

        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readGapconductance(MechContactData::mContactBase* contact)
    {
        contact->heat_define = true;
        contact->heat_value.clear();
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*"))
                break;

            if (currentline.isEmpty())
                continue;

            _linelist = currentline.split(",");
            double ratio = getQStringListIndexValue(_linelist, 0).toDouble();
            double clearance = getQStringListIndexValue(_linelist, 1).toDouble();
            contact->heat_value.push_back({ ratio ,clearance });
        }
    }
    void mIMxFile::readAdjust(MechContactData::mContactBase* contact)
    {
        contact->geom_define = true;
        currentline = _file.readLine().simplified();
        contact->geom_value = currentline.toDouble();
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readClearance(MechContactData::mContactBase* contact)
    {
        contact->gap_define = true;
        currentline = _file.readLine().simplified();
        contact->gap_overclosure = currentline.toDouble();
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readEvaluation(MechContactData::mContactBase* contact)
    {
        contact->evaluation = true;
        currentline = _file.readLine().simplified();
        QStringList evalList = currentline.split(",");
        contact->eva_MP = false;
        contact->eva_SD = false;
        contact->eva_CV = false;
        contact->eva_PC = false;
        contact->eva_R = false;
        if (evalList[0].trimmed() == "1") contact->eva_MP = true;
        if (evalList[1].trimmed() == "1") contact->eva_SD = true;
        if (evalList[2].trimmed() == "1") contact->eva_CV = true;
        if (evalList[3].trimmed() == "1") contact->eva_PC = true;
        if (evalList[4].trimmed() == "1")  contact->eva_R = true;
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readNodalload()
    {
        QString boltload = getKeyValue(currentline, "boltload", "=");
        if (!boltload.compare("on", Qt::CaseInsensitive))
        {
            readBoltload();
            return;
        }

        // 解析行参数
        QString type = getKeyValue(currentline, "type", "=");
        QString file = getKeyValue(currentline, "file", "=");
        QString name = getKeyValue(currentline, "name", "=");
        QString set = getKeyValue(currentline, "set", "=");
        QString function = getKeyValue(currentline, "function", "=");
        QString csys = getKeyValue(currentline, "csys", "=");
        QString field = getKeyValue(currentline, "field", "=");

        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);
        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();

        // 创建载荷对象
        shared_ptr<MechCaseData::mLoadForce> load = make_shared<MechCaseData::mLoadForce>(loadID, LOAD_IM_FORCE, name);
        bool isTorque = !type.compare("moment", Qt::CaseInsensitive);
        if (isTorque)
        {
            load->setLoadBoundaryType(LOAD_IM_TORQUE);
        }
        _modeldata->addPhysicalFieldP(load);

        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Coordinate, csys);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Field, field, MapRelationType::MapRelation_AnalysisField);

        auto creatMoment = [&]() {
            QString name_2 = QString("%1_moment").arg(name);
            name_2 = ExistRenamePhysicalName(Physical_LoadBoundary, name_2);
            int loadID_2 = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
            shared_ptr<MechCaseData::mLoadForce> load2 = make_shared<MechCaseData::mLoadForce>(loadID_2, LOAD_IM_TORQUE, name_2);
            _modeldata->addPhysicalFieldP(load2);
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID_2, Physical_Function, function);
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID_2, Physical_Coordinate, csys);
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID_2, Physical_Set, set);
            return load2;
            };

        if (!field.isEmpty())
        {
            // 分析场：way_ 保持各向分量语义，独立标志记录分析场方式，无数据行
            load->isAnalysisFieldMode_ = true;
            load->way_ = 0;
            currentline = _file.readLine().simplified();
        }
        else if (!file.isEmpty())
        {
            load->way_ = isTorque ? 2 : 3;

            QFileInfo fi(_filename);
            QString dirPath = fi.absolutePath();
            load->path_ = dirPath + "/" + file;

            currentline = _file.readLine().simplified();
        }
        else if (!type.isEmpty())
        {
            if (!set.isEmpty())
            {
                load->way_ = 0; // 力和力矩统一反显为各向分量

                appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

                // 读取三个分量（单位制换算：文件值(当前界面单位制) → 内部）
                const MxFunctions::Unit loadUnit = isTorque ? MxFunctions::Unit::Torque : MxFunctions::Unit::Force;
                currentline = _file.readLine().simplified();
                if (!currentline.isEmpty() && !currentline.startsWith("*"))
                {
                    _linelist = currentline.split(",");

                    QString value = getQStringListIndexValue(_linelist, 0).trimmed();
                    if (!value.isEmpty())
                        load->force_x = mUnitIoCodec::decode(loadUnit, value.toDouble());

                    value = getQStringListIndexValue(_linelist, 1).trimmed();
                    if (!value.isEmpty())
                        load->force_y = mUnitIoCodec::decode(loadUnit, value.toDouble());

                    value = getQStringListIndexValue(_linelist, 2).trimmed();
                    if (!value.isEmpty())
                        load->force_z = mUnitIoCodec::decode(loadUnit, value.toDouble());
                }

                currentline = _file.readLine().simplified();
            }
            else
            {
                // 独立赋值：集中力→way_=2；力矩→way_=1
                load->way_ = isTorque ? 1 : 2;
                load->nodeForceMap_.clear();

                // 循环读取所有节点力/力矩行
                currentline = _file.readLine().simplified();
                while (!currentline.isEmpty() && !currentline.startsWith("*"))
                {
                    _linelist = currentline.split(",");
                    if (_linelist.size() >= 3)
                    {
                        int nodeId = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxNodeID;
                        int freedom = getQStringListIndexValue(_linelist, 1).trimmed().toInt();
                        // 力矩离散型数据：4=X, 5=Y, 6=Z → 内部映射为 0,1,2
                        if (isTorque && freedom >= 4 && freedom <= 6)
                            freedom -= 4;
                        else
                            freedom -= 1;
                        //单位制换算：文件值(当前界面单位制) → 内部（力/力矩）
                        double value = mUnitIoCodec::decode(isTorque ? MxFunctions::Unit::Torque : MxFunctions::Unit::Force,
                            getQStringListIndexValue(_linelist, 2).trimmed().toDouble());
                        RenderPickResultData key(nodeId, -1, -1);
                        auto it = load->nodeForceMap_.find(key);
                        if (it == load->nodeForceMap_.end())
                        {
                            load->nodeForceMap_[key] = QVector<QVariant>(3);
                            it = load->nodeForceMap_.find(key);
                        }
                        it->second[freedom] = value;
                    }

                    currentline = _file.readLine().simplified();
                }
            }
        }
        else
        {
            if (!set.isEmpty())
            {
                load->way_ = 0; // 力和力矩统一反显为各向分量

                appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

                // 读取三个分量
                currentline = _file.readLine().simplified();
                if (!currentline.isEmpty() && !currentline.startsWith("*"))
                {
                    _linelist = currentline.split(",");

                    QString x_value = getQStringListIndexValue(_linelist, 0).trimmed();
                    QString y_value = getQStringListIndexValue(_linelist, 1).trimmed();
                    QString z_value = getQStringListIndexValue(_linelist, 2).trimmed();
                    QString rx_value = getQStringListIndexValue(_linelist, 3).trimmed();
                    QString ry_value = getQStringListIndexValue(_linelist, 4).trimmed();
                    QString rz_value = getQStringListIndexValue(_linelist, 5).trimmed();

                    //单位制换算：文件值(当前界面单位制) → 内部（力分量/力矩分量，空值原样保留）
                    auto decodeStr = [](const QString& s, const MxFunctions::Unit& u)->QString {
                        bool ok = false; double v = s.toDouble(&ok);
                        return (ok && !s.simplified().isEmpty()) ? MxFunctions::mNumberToString(mUnitIoCodec::decode(u, v)) : s;
                    };
                    x_value = decodeStr(x_value, MxFunctions::Unit::Force);
                    y_value = decodeStr(y_value, MxFunctions::Unit::Force);
                    z_value = decodeStr(z_value, MxFunctions::Unit::Force);
                    rx_value = decodeStr(rx_value, MxFunctions::Unit::Torque);
                    ry_value = decodeStr(ry_value, MxFunctions::Unit::Torque);
                    rz_value = decodeStr(rz_value, MxFunctions::Unit::Torque);

                    int load_type = 0;
                    if (!(rx_value.isEmpty() && ry_value.isEmpty() && rz_value.isEmpty()) && (rx_value.toDouble() + ry_value.toDouble() + rz_value.toDouble() != 0))
                    {
                        load_type++;
                        load->setLoadBoundaryType(LOAD_IM_TORQUE);
                    }
                    if (!(x_value.isEmpty() && y_value.isEmpty() && z_value.isEmpty()) && (x_value.toDouble() + y_value.toDouble() + z_value.toDouble() != 0))
                    {
                        load_type++;
                        load->setLoadBoundaryType(LOAD_IM_FORCE);
                    }

                    if (load_type == 1 && load->getLoadBoundaryType() == LOAD_IM_FORCE)
                    {
                        if (!x_value.isEmpty()) load->force_x = x_value.toDouble();
                        if (!y_value.isEmpty()) load->force_y = y_value.toDouble();
                        if (!z_value.isEmpty()) load->force_z = z_value.toDouble();
                    }
                    else if (load_type == 1 && load->getLoadBoundaryType() == LOAD_IM_TORQUE)
                    {
                        if (!rx_value.isEmpty()) load->force_x = rx_value.toDouble();
                        if (!ry_value.isEmpty()) load->force_y = ry_value.toDouble();
                        if (!rz_value.isEmpty()) load->force_z = rz_value.toDouble();
                    }
                    else if (load_type == 2)
                    {
                        QString name_1 = QString("%1_force").arg(name);
                        name_1 = ExistRenamePhysicalName(Physical_LoadBoundary, name_1);
                        load->setLoadBoundaryName(name_1);

                        if (!x_value.isEmpty()) load->force_x = x_value.toDouble();
                        if (!y_value.isEmpty()) load->force_y = y_value.toDouble();
                        if (!z_value.isEmpty()) load->force_z = z_value.toDouble();

                        shared_ptr<MechCaseData::mLoadForce> load2 = creatMoment();
                        load2->way_ = 0;
                        if (!rx_value.isEmpty()) load2->force_x = rx_value.toDouble();
                        if (!ry_value.isEmpty()) load2->force_y = ry_value.toDouble();
                        if (!rz_value.isEmpty()) load2->force_z = rz_value.toDouble();

                        std::vector<std::pair<int, double>> caseData =_mapCaseValue[Physical_LoadBoundary][name];
                        _mapCaseValue[Physical_LoadBoundary].remove(name);
                        _mapCaseValue[Physical_LoadBoundary][load->getLoadBoundaryName()] = caseData;
                        _mapCaseValue[Physical_LoadBoundary][load2->getLoadBoundaryName()] = caseData;
                    }
                }

                currentline = _file.readLine().simplified();
            }
            else
            {
                std::map<RenderPickResultData, QVector<QVariant>> forceData;
                std::map<RenderPickResultData, QVector<QVariant>> momentData;

                // 循环读取所有节点力/力矩行
                currentline = _file.readLine().simplified();
                while (!currentline.isEmpty() && !currentline.startsWith("*"))
                {
                    _linelist = currentline.split(",");
                    if (_linelist.size() >= 3)
                    {
                        int nodeId = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxNodeID;
                        int freedom = getQStringListIndexValue(_linelist, 1).trimmed().toInt();
                        //单位制换算：文件值(当前界面单位制) → 内部（自由度4-6为力矩，1-3为力）
                        double value = mUnitIoCodec::decode(freedom >= 4 && freedom <= 6 ? MxFunctions::Unit::Torque : MxFunctions::Unit::Force,
                            getQStringListIndexValue(_linelist, 2).trimmed().toDouble());
                        RenderPickResultData key(nodeId, -1, -1);
                        // 力矩离散型数据：4=X, 5=Y, 6=Z → 内部映射为 0,1,2
                        if (freedom >= 4 && freedom <= 6)
                        {
                            freedom -= 4;
                            auto it = momentData.find(key);
                            if (it == momentData.end())
                            {
                                momentData[key] = QVector<QVariant>(3);
                                it = momentData.find(key);
                            }
                            it->second[freedom] = value;
                        }
                        else
                        {
                            freedom -= 1;
                            auto it = forceData.find(key);
                            if (it == forceData.end())
                            {
                                forceData[key] = QVector<QVariant>(3);
                                it = forceData.find(key);
                            }
                            it->second[freedom] = value;
                        }
                    }

                    currentline = _file.readLine().simplified();
                }

                // 独立赋值：集中力→way_=2；力矩→way_=1
                if (forceData.empty() && !momentData.empty())
                {
                    load->setLoadBoundaryType(LOAD_IM_TORQUE);
                    load->way_ = 1;
                    load->nodeForceMap_ = momentData;
                }
                else if (!forceData.empty() && momentData.empty())
                {
                    load->setLoadBoundaryType(LOAD_IM_FORCE);
                    load->way_ = 2;
                    load->nodeForceMap_ = forceData;
                }
                else
                {
                    QString name_1 = QString("%1_force").arg(name);
                    name_1 = ExistRenamePhysicalName(Physical_LoadBoundary, name_1);
                    load->setLoadBoundaryName(name_1);
                    load->way_ = 2;
                    load->nodeForceMap_ = forceData;

                    shared_ptr<MechCaseData::mLoadForce> load2 = creatMoment();
                    load2->way_ = 1;
                    load2->nodeForceMap_ = momentData;

                    std::vector<std::pair<int, double>> caseData = _mapCaseValue[Physical_LoadBoundary][name];
                    _mapCaseValue[Physical_LoadBoundary].remove(name);
                    _mapCaseValue[Physical_LoadBoundary][load->getLoadBoundaryName()] = caseData;
                    _mapCaseValue[Physical_LoadBoundary][load2->getLoadBoundaryName()] = caseData;
                }
            }
        }
    }
    void mIMxFile::readBodyload()
    {
        QString type = getKeyValue(currentline, "type", "=");
        QString set = getKeyValue(currentline, "set", "=");

        if (!type.compare("gravity", Qt::CaseInsensitive))
        {
            QString name = getKeyValue(currentline, "name", "=");
            name = ExistRenamePhysicalName(Physical_LoadBoundary, name);
            QString function = getKeyValue(currentline, "function", "=");

            int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
            shared_ptr<MechCaseData::mLoadGravity> load = make_shared<MechCaseData::mLoadGravity>(loadID, LOAD_IM_GRAVITY, name);
            _modeldata->addPhysicalFieldP(load);

            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);

            if (!set.isEmpty())
            {
                appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);
                load->isCornerAddChecked = true;
            }

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            QString value = getQStringListIndexValue(_linelist, 0);
            if (!value.isEmpty()) load->V1 = value.toDouble();
            value = getQStringListIndexValue(_linelist, 1);
            if (!value.isEmpty()) load->V2 = value.toDouble();
            value = getQStringListIndexValue(_linelist, 2);
            if (!value.isEmpty()) load->V3 = value.toDouble();

            // 新格式：统一按各向分量读取（way_ = 0），并同步 dir_ 用于面板反显
            load->way_ = 0;
            QVector3D dir;
            if (load->V1.isValid() && !load->V1.isNull()) dir.setX(load->V1.toDouble());
            if (load->V2.isValid() && !load->V2.isNull()) dir.setY(load->V2.toDouble());
            if (load->V3.isValid() && !load->V3.isNull()) dir.setZ(load->V3.toDouble());
            if (!dir.isNull())
            {
                dir.normalize();
                load->dir_ = dir;
            }

            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("internalheat", Qt::CaseInsensitive))  //热流密度->体热流
        {
            QString name = getKeyValue(currentline, "name", "=");
            name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

            QString set = getKeyValue(currentline, "set", "=");
            QString function = getKeyValue(currentline, "function", "=");

            int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
            shared_ptr<MechCaseData::mLoadHeatFlux> load = make_shared<MechCaseData::mLoadHeatFlux>(loadID, LOAD_IM_HEATFLUX, name);
            load->way_ = 2;
            _modeldata->addPhysicalFieldP(load);

            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);

            // ========== 核心：用set参数区分统一/独立赋值 ==========
            if (!set.isEmpty())
            {
                // 统一赋值（原有逻辑完全不变）
                load->defineType_ = 0;
                appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                if (_linelist.size() >= 1)
                {
                    QString value = getQStringListIndexValue(_linelist, 0).trimmed();
                    if (!value.isEmpty())
                    {
                        load->pa_ = value.toDouble();
                    }
                }
                currentline = _file.readLine().simplified();
            }
            else
            {
                // 独立赋值（新增）
                load->defineType_ = 1;
                load->cellFaceData_.clear();

                currentline = _file.readLine().simplified();
                while (!currentline.isEmpty() && !currentline.startsWith("*"))
                {
                    _linelist = currentline.split(",");
                    if (_linelist.size() >= 2)
                    {
                        int cellId = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxMeshID;
                        double val = getQStringListIndexValue(_linelist, 1).trimmed().toDouble();
                        int partID = -1;
                        if (_elementIDToPartID.contains(cellId))
                            partID = _elementIDToPartID[cellId];
                        load->cellFaceData_[RenderPickResultData(cellId, partID, 0)] = { QVariant(val) }; // 体热流次ID固定为0
                    }
                    currentline = _file.readLine().simplified();
                }
            }
        }
        else if (!type.compare("centrifugalload", Qt::CaseInsensitive)
            || !type.compare("centload", Qt::CaseInsensitive)
            || !type.compare("rotaryaccelerationload", Qt::CaseInsensitive))
        {
            QString name = getKeyValue(currentline, "name", "=");
            name = ExistRenamePhysicalName(Physical_LoadBoundary, name);
            QString function = getKeyValue(currentline, "function", "=");

            int rotateType = 0;
            if (!type.compare("centrifugalload", Qt::CaseInsensitive))
                rotateType = 0;
            else if (!type.compare("rotaryaccelerationload", Qt::CaseInsensitive))
                rotateType = 1;

            int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
            shared_ptr<MechCaseData::mRotatePhysicalPower> load = make_shared<MechCaseData::mRotatePhysicalPower>(loadID, LOAD_ROTATE_PHYSICAL_POWER, name);
            load->_rotateType = rotateType;
            _modeldata->addPhysicalFieldP(load);

            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);



            if(!set.isEmpty())
                appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            if (_linelist.size() <= 7)
            {
                // 统一赋值
                load->_defineType = 0;
                QString value = getQStringListIndexValue(_linelist, 0).trimmed();
                if (!value.isEmpty())
                {
                    double omega2 = value.toDouble();
                    // 离心力类型时，mx 中保存的是 rad/s 角速度平方，反算为 rad/s 存储
                    if (rotateType == 0 && omega2 > 0)
                    {
                        double omega = sqrt(omega2);
                        load->_uniformValue = omega;
                    }
                    else
                    {
                        load->_uniformValue = omega2;
                    }
                }

                load->_startPoint1.setX(getQStringListIndexValue(_linelist, 1).trimmed().toDouble());
                load->_startPoint1.setY(getQStringListIndexValue(_linelist, 2).trimmed().toDouble());
                load->_startPoint1.setZ(getQStringListIndexValue(_linelist, 3).trimmed().toDouble());

                load->_directPoint2.setX(getQStringListIndexValue(_linelist, 4).trimmed().toDouble());
                load->_directPoint2.setY(getQStringListIndexValue(_linelist, 5).trimmed().toDouble());
                load->_directPoint2.setZ(getQStringListIndexValue(_linelist, 6).trimmed().toDouble());

                // 统一使用 DirectInput 模式保存方向控件状态，便于面板反显
                load->_dirPickState = DirectionPickState();
                load->_dirPickState.mode = 6;
                load->_dirPickState.directInputDir = load->_directPoint2 - load->_startPoint1;

                currentline = _file.readLine().simplified();
            }

            else
            {
                // 独立赋值
                load->_defineType = 1;
                load->_elementCoorMap.clear();

                while (!currentline.isEmpty() && !currentline.startsWith("*"))
                {
                    _linelist = currentline.split(",");
                    if (_linelist.size() >= 8)
                    {
                        int elementId = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxMeshID;
                        QString val = getQStringListIndexValue(_linelist, 1).trimmed();
                        QString startPoint = QString("%1, %2, %3")
                            .arg(getQStringListIndexValue(_linelist, 2).trimmed())
                            .arg(getQStringListIndexValue(_linelist, 3).trimmed())
                            .arg(getQStringListIndexValue(_linelist, 4).trimmed());
                        QString directPoint = QString("%1, %2, %3")
                            .arg(getQStringListIndexValue(_linelist, 5).trimmed())
                            .arg(getQStringListIndexValue(_linelist, 6).trimmed())
                            .arg(getQStringListIndexValue(_linelist, 7).trimmed());

                        int partID = -1;
                        if (_elementIDToPartID.contains(elementId))
                            partID = _elementIDToPartID[elementId];

                        QVector<QVariant> values(3);
                        if (!val.isEmpty())
                            values[0] = QVariant(val.toDouble());
                        values[1] = QVariant::fromValue(QVector3D(
                            getQStringListIndexValue(_linelist, 2).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 3).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 4).trimmed().toDouble()));
                        values[2] = QVariant::fromValue(QVector3D(
                            getQStringListIndexValue(_linelist, 5).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 6).trimmed().toDouble(),
                            getQStringListIndexValue(_linelist, 7).trimmed().toDouble()));

                        load->_elementCoorMap[RenderPickResultData(elementId, partID, -1)] = std::move(values);
                    }
                    currentline = _file.readLine().simplified();
                }
            }
        }
        else
        {
            currentline = _file.readLine().simplified();
        }

    }
    void mIMxFile::readSurfaceload()
    {
        QString type = getKeyValue(currentline, "type", "=");
        if (!type.compare("pressure", Qt::CaseInsensitive) || !type.compare("hydrostatic", Qt::CaseInsensitive))
        {
            readPressure();
        }
        else if (!type.compare("flux", Qt::CaseInsensitive)) //热流密度->面热流
        {
            readHeatFlux();
        }
        else if (!type.compare("radiation", Qt::CaseInsensitive))
        {
            readRadiation();
        }
        else if (!type.compare("conv", Qt::CaseInsensitive))
        {
            readConvection();
        }
        else
        {
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readSurface()
    {
        // 1. 解析行头参数
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString set = getKeyValue(currentline, "set", "=");
        QString function = getKeyValue(currentline, "function", "=");
        QString direction = getKeyValue(currentline, "direction", "=").trimmed().toLower();

        // 2. 创建载荷对象
        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        auto load = std::make_shared<MechCaseData::mLoadSurface>(loadID, LOAD_IM_SURFACE, name);
        load->distributeType_ = 0; // 默认均匀分布
        _modeldata->addPhysicalFieldP(load);

        // 3. 绑定函数映射
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);

        // 方向模式映射
        if (direction == "shear")
            load->dirMode_ = 0;      // 表面切向
        else if (direction == "trvec")
            load->dirMode_ = 1;      // 通用
        else
            load->dirMode_ = 0;      // 默认表面切向

        // 4. 按set区分统一/独立赋值
        if (!set.isEmpty())
        {
            // ========== 统一赋值 ==========
            load->way_ = 0;
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

            // 读取数值行：Val, V1, V2, V3
            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");

            // 压强值
            if (_linelist.size() >= 1)
            {
                QString valStr = getQStringListIndexValue(_linelist, 0).trimmed();
                if (!valStr.isEmpty())
                {
                    load->loadValue_ = QVariant::fromValue<double>(valStr.toDouble());
                }
            }

            // 方向三分量，空值保持零向量
            double v1 = 0.0, v2 = 0.0, v3 = 0.0;
            bool hasValidDir = false;

            if (_linelist.size() >= 2)
            {
                QString v1Str = getQStringListIndexValue(_linelist, 1).trimmed();
                if (!v1Str.isEmpty()) { v1 = v1Str.toDouble(); hasValidDir = true; }
            }
            if (_linelist.size() >= 3)
            {
                QString v2Str = getQStringListIndexValue(_linelist, 2).trimmed();
                if (!v2Str.isEmpty()) { v2 = v2Str.toDouble(); hasValidDir = true; }
            }
            if (_linelist.size() >= 4)
            {
                QString v3Str = getQStringListIndexValue(_linelist, 3).trimmed();
                if (!v3Str.isEmpty()) { v3 = v3Str.toDouble(); hasValidDir = true; }
            }

            // 统一使用 DirectInput 模式保存方向，便于方向控件反显
            load->dirPickState_ = DirectionPickState();
            load->dirPickState_.mode = 6;
            load->dirPickState_.directInputDir = hasValidDir ? QVector3D(v1, v2, v3) : QVector3D();

            // 读取下一行，交给外层流程继续解析
            currentline = _file.readLine().simplified();
        }
        else
        {
            // ========== 独立赋值 ==========
            load->way_ = 1;
            load->surfaceForceMap_.clear();

            currentline = _file.readLine().simplified();
            while (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                _linelist = currentline.split(",");
                // 格式：EID, B<面号>, Val, V1, V2, V3
                if (_linelist.size() >= 3)
                {
                    // 单元ID + 网格偏移
                    int cellId = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxMeshID;

                    // 去除面ID前缀B，兼容大小写
                    QString faceStr = getQStringListIndexValue(_linelist, 1).trimmed();
                    if (faceStr.startsWith('B', Qt::CaseInsensitive))
                        faceStr = faceStr.mid(1);
                    int faceId = faceStr.toInt();

                    // 提取四个字段，不足则为空 QVariant
                    auto stringToVariant = [](const QString& s) -> QVariant {
                        if (s.trimmed().isEmpty()) return QVariant();
                        return QVariant(s.toDouble());
                    };

                    QString valStr = getQStringListIndexValue(_linelist, 2).trimmed();
                    QString v1Str = _linelist.size() >= 4 ? getQStringListIndexValue(_linelist, 3).trimmed() : "";
                    QString v2Str = _linelist.size() >= 5 ? getQStringListIndexValue(_linelist, 4).trimmed() : "";
                    QString v3Str = _linelist.size() >= 6 ? getQStringListIndexValue(_linelist, 5).trimmed() : "";

                    // 获取部件ID
                    int partID = -1;
                    if (_elementIDToPartID.contains(cellId))
                        partID = _elementIDToPartID[cellId];

                    // 存入map
                    QVector<QVariant> values(4);
                    values[0] = stringToVariant(valStr);
                    values[1] = stringToVariant(v1Str);
                    values[2] = stringToVariant(v2Str);
                    values[3] = stringToVariant(v3Str);
                    load->surfaceForceMap_[RenderPickResultData(cellId, partID, faceId)] = values;
                }
                currentline = _file.readLine().simplified();
            }
        }
    }

    void mIMxFile::readNodalflux()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString set = getKeyValue(currentline, "set", "=");
        QString function = getKeyValue(currentline, "function", "=");

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadHeatFlux> load = make_shared<MechCaseData::mLoadHeatFlux>(loadID, LOAD_IM_HEATFLUX, name);
        load->way_ = 0;
        _modeldata->addPhysicalFieldP(load);

        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);

        if (!set.isEmpty())
        {
            // 统一赋值（原有逻辑完全不变）
            load->defineType_ = 0;
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            if (_linelist.size() >= 1)
            {
                QString value = getQStringListIndexValue(_linelist, 0).trimmed();
                if (!value.isEmpty())
                {
                    load->pa_ = value.toDouble();
                }
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            // 独立赋值（新增）
            load->defineType_ = 1;
            load->cellFaceData_.clear();

            currentline = _file.readLine().simplified();
            while (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                _linelist = currentline.split(",");
                if (_linelist.size() >= 2)
                {
                    int nodeId = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxNodeID;
                    double val = getQStringListIndexValue(_linelist, 1).trimmed().toDouble();
                    load->cellFaceData_[RenderPickResultData(nodeId, -1, 0)] = { QVariant(val) }; // 点热流次ID固定为0
                }
                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::readTransient()
    {
        QString type = getKeyValue(currentline, "type", "=");

        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadTransient> load = make_shared<MechCaseData::mLoadTransient>(loadID, LOAD_IM_TRANSIENT, name);

        if (!type.compare("load", Qt::CaseInsensitive))
        {
            load->way_ = 0;
        }
        else if (!type.compare("disp", Qt::CaseInsensitive))
        {
            load->way_ = 1;
        }
        else if (!type.compare("vel", Qt::CaseInsensitive))
        {
            load->way_ = 2;
        }
        else if (!type.compare("acc", Qt::CaseInsensitive))
        {
            load->way_ = 3;
        }

        double delay = getKeyValue(currentline, "delay", "=").toDouble();
        load->pa_ = delay;

        //载荷名称读文件待定,目前往回读无法映射
        QString loadName = _file.readLine().simplified();
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_LoadBoundary, loadName.remove('"'));
        _modeldata->addPhysicalFieldP(load);
        currentline = _file.readLine().simplified();
    }

    //读旋转加速度
    void mIMxFile::readRoACC()
    {
        // 每行格式：数据列... , omegaX, omegaY, omegaZ, startX, startY, startZ
        // 后 3 列为起始点 _startPoint1；其前 3 列为 omega 分量。
        // 旋转加速度大小 norm = |omega|，方向点 = start + omega / norm。
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_RotateAnalyse, name);
        int loadID = _modeldata->
            getManager<MechCaseData::mRotateAnalyseManager>(Physical_RotateAnalyse).createID();
        shared_ptr<MechCaseData::mLoadRotationalACC> load
            = make_shared<MechCaseData::mLoadRotationalACC>
            (loadID, ROTATIONAL_ACC, name);
        _modeldata->addPhysicalFieldP(load);

        QString part = getKeyValue(currentline, "part", "=");
        if (!part.isEmpty())
        {
            load->_rotationalPart = part;
            appendMapPhysicalStruct(Physical_RotateAnalyse, loadID, Physical_Set, part);
        }

        QString type = getKeyValue(currentline, "type", "=");
        int accCurveType = 0;
        if (!type.compare("time", Qt::CaseInsensitive))
            accCurveType = 1;
        load->_accCurveType = accCurveType;

        currentline = _file.readLine().simplified();
        QList<QStringList> tableData;
        while (!currentline.isEmpty() && !currentline.startsWith("*"))
        {
            _linelist = currentline.split(",");
            if (_linelist.size() < 6)
            {
                currentline = _file.readLine().simplified();
                continue;
            }

            // 最后 3 列为起始点
            QVector3D startP;
            startP.setX(getQStringListIndexValue(_linelist, _linelist.size() - 3).toFloat());
            startP.setY(getQStringListIndexValue(_linelist, _linelist.size() - 2).toFloat());
            startP.setZ(getQStringListIndexValue(_linelist, _linelist.size() - 1).toFloat());
            load->_startPoint1 = startP;

            // 起始点之前的 3 列为 omega 分量
            QVector3D omega;
            omega.setX(getQStringListIndexValue(_linelist, _linelist.size() - 6).toFloat());
            omega.setY(getQStringListIndexValue(_linelist, _linelist.size() - 5).toFloat());
            omega.setZ(getQStringListIndexValue(_linelist, _linelist.size() - 4).toFloat());

            double norm = std::sqrt(omega.x() * omega.x()
                + omega.y() * omega.y()
                + omega.z() * omega.z());

            QVector3D direP;
            double inv = 0.;
            if (norm != 0.0)
            {
                inv = 1.0 / norm;
                direP.setX( omega.x() * inv);
                direP.setY(omega.y() * inv);
                direP.setZ(omega.z() * inv);

            }
            load->_directPoint2 = startP + direP;

            // 统一使用 DirectInput 模式保存方向控件状态，便于面板反显
            load->_dirPickState = DirectionPickState();
            load->_dirPickState.mode = 6;
            load->_dirPickState.directInputDir = omega;

            // 表格中不再输入起始点/方向点，表格仅保留数据列；起始点/方向点由控件获取
            QStringList row;
            if (accCurveType == 1)
            {
                // 时间曲线：第 1 列为时间
                row << getQStringListIndexValue(_linelist, 0).trimmed();
            }
            row << QString("%1").arg(norm, 0, 'g', 16);

            tableData << row;
            currentline = _file.readLine().simplified();
        }

        if (accCurveType == 0)
        {
            load->_constantTableHeaders = QStringList{ "旋转加速度" };
            load->_constantTableData = tableData;
            load->_tableHeaders = load->_constantTableHeaders;
            load->_tableData = tableData;
        }
        else
        {
            load->_timeCurveTableHeaders = QStringList{ "时间", "旋转加速度" };
            load->_timeCurveTableData = tableData;
            load->_tableHeaders = load->_timeCurveTableHeaders;
            load->_tableData = tableData;
        }
    }

    //读旋转速度
    void mIMxFile::readRoSpeed()
    {
        // 每行格式：数据列... , omegaX, omegaY, omegaZ, startX, startY, startZ
        // 后 3 列为起始点 _startPoint1；其前 3 列为 omega 分量。
        // 旋转速度大小 norm = |omega|，方向点 = start + omega / norm。
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_RotateAnalyse, name);
        int loadID = _modeldata->
            getManager<MechCaseData::mRotateAnalyseManager>(Physical_RotateAnalyse).createID();
        shared_ptr<MechCaseData::mLoadRotationalSpeed> load
            = make_shared<MechCaseData::mLoadRotationalSpeed>
            (loadID, ROTATIONAL_SPEED, name);
        _modeldata->addPhysicalFieldP(load);

        QString part = getKeyValue(currentline, "part", "=");
        if (!part.isEmpty())
        {
            load->_rotationalPart = part;
            appendMapPhysicalStruct(Physical_RotateAnalyse, loadID, Physical_Set, part);
        }

        QString type = getKeyValue(currentline, "type", "=");
        int speedCurveType = 0;
        if (!type.compare("table", Qt::CaseInsensitive))
            speedCurveType = 1;
        else if (!type.compare("time", Qt::CaseInsensitive))
            speedCurveType = 2;
        load->_speedCurveType = speedCurveType;

        currentline = _file.readLine().simplified();
        QList<QStringList> tableData;
        while (!currentline.isEmpty() && !currentline.startsWith("*"))
        {
            _linelist = currentline.split(",");
            if (_linelist.size() < 6)
            {
                currentline = _file.readLine().simplified();
                continue;
            }

            // 最后 3 列为起始点
            QVector3D startP;
            startP.setX(getQStringListIndexValue(_linelist, _linelist.size() - 3).toFloat());
            startP.setY(getQStringListIndexValue(_linelist, _linelist.size() - 2).toFloat());
            startP.setZ(getQStringListIndexValue(_linelist, _linelist.size() - 1).toFloat());
            load->_startPoint1 = startP;

            // 起始点之前的 3 列为 omega 分量
            QVector3D omega;
            omega.setX(getQStringListIndexValue(_linelist, _linelist.size() - 6).toFloat());
            omega.setY(getQStringListIndexValue(_linelist, _linelist.size() - 5).toFloat());
            omega.setZ(getQStringListIndexValue(_linelist, _linelist.size() - 4).toFloat());

            double norm = std::sqrt(omega.x() * omega.x()
                + omega.y() * omega.y()
                + omega.z() * omega.z());

            QVector3D direP;
            double inv = 0.;
            if (norm != 0.0)
            {
                inv = 1.0 / norm;
                direP.setX(omega.x() * inv);
                direP.setY(omega.y() * inv);
                direP.setZ(omega.z() * inv);

            }
            load->_directPoint2 = startP + direP;

            // 统一使用 DirectInput 模式保存方向控件状态，便于面板反显
            load->_dirPickState = DirectionPickState();
            load->_dirPickState.mode = 6;
            load->_dirPickState.directInputDir = omega;

            // 表格中不再输入起始点/方向点，表格仅保留数据列；起始点/方向点由控件获取
            QStringList row;
            if (speedCurveType == 2)
            {
                // 时间曲线：第 1 列为时间
                row << getQStringListIndexValue(_linelist, 0).trimmed();
            }
            row << QString("%1").arg(norm, 0, 'g', 16);

            tableData << row;
            currentline = _file.readLine().simplified();
        }

        if (speedCurveType == 0)
        {
            load->_constantTableHeaders = QStringList{ "旋转速度" };
            load->_constantTableData = tableData;
            load->_tableHeaders = load->_constantTableHeaders;
            load->_tableData = tableData;
        }
        else if (speedCurveType == 1)
        {
            load->_curveTableHeaders = QStringList{ "旋转速度" };
            load->_curveTableData = tableData;
            load->_tableHeaders = load->_curveTableHeaders;
            load->_tableData = tableData;
        }
        else
        {
            load->_timeCurveTableHeaders = QStringList{ "时间", "旋转速度" };
            load->_timeCurveTableData = tableData;
            load->_tableHeaders = load->_timeCurveTableHeaders;
            load->_tableData = tableData;
        }
    }

    void mIMxFile::readEdgeload()
    {
        // 1. 解析头部行关键字段
        QString type = getKeyValue(currentline, "type", "=");
        QString name = getKeyValue(currentline, "name", "=");
        QString direction = getKeyValue(currentline, "direction", "=");
        QString set = getKeyValue(currentline, "set", "=");
        QString function = getKeyValue(currentline, "function", "=");

        // 2. 名称重命名 + 创建载荷ID与对象
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);
        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary)
            .createID();

        auto load = std::make_shared<MechCaseData::mLoadEdge>(loadID, LOAD_IM_EDGE, name);
        _modeldata->addPhysicalFieldP(load);

        // 3. 建立集合、函数映射关系
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);

        // 4. 方向模式：英文标识转内部中文（与getDirectionModeEng一一对应）
        static const QMap<QString, QString> dirEngToCn = {
            {"ednor", "面内法向"},
            {"edtra", "面外法向"},
            {"edshr", "边切向"},
            {"edld",    "通用"}
        };
        load->directionMode_ = dirEngToCn.value(direction, "面内法向"); // 默认兜底

        // 5. 读取数值行
        currentline = _file.readLine().simplified();
        if (!currentline.isEmpty() && !currentline.startsWith("*"))
        {
            _linelist = currentline.split(",");

            if (direction == "edld") // 通用模式：Val, V1, V2, V3
            {
                // 读取载荷值
                if (_linelist.size() > 0)
                {
                    load->size_ = getQStringListIndexValue(_linelist, 0).trimmed();
                }

                // 读取方向矢量三分量
                if (_linelist.size() >= 4)
                {
                    bool v1Ok = false, v2Ok = false, v3Ok = false;
                    double v1 = getQStringListIndexValue(_linelist, 1).trimmed().toDouble(&v1Ok);
                    double v2 = getQStringListIndexValue(_linelist, 2).trimmed().toDouble(&v2Ok);
                    double v3 = getQStringListIndexValue(_linelist, 3).trimmed().toDouble(&v3Ok);

                    if (v1Ok && v2Ok && v3Ok)
                    {
                        QVector3D dir(v1, v2, v3);
                        if (!dir.isNull())
                        {
                            dir.normalize();
                            load->dir_ = dir;

                            // 同步设置方向控件状态：标记为直接输入模式，反显时自动切到对应页
                            load->dirPickState_.mode = 6;
                            load->dirPickState_.reversed = false;
                            load->dirPickState_.directInputDir = dir;
                        }
                    }
                }
            }
            else // 非通用模式：仅载荷值
            {
                if (_linelist.size() > 0)
                {
                    load->size_ = getQStringListIndexValue(_linelist, 0).trimmed();
                }
            }

            // 6. 推进到下一行，准备解析下一个条目
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readFreqdomain(QString loadType)
    {
        QString type = getKeyValue(currentline, "type", "=");

        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadFreqdomain> load = make_shared<MechCaseData::mLoadFreqdomain>(loadID, LOAD_IM_FREQDOMAIN, name);

        if (!type.compare("load", Qt::CaseInsensitive))
        {
            load->way_ = 0;
        }
        else if (!type.compare("disp", Qt::CaseInsensitive))
        {
            load->way_ = 1;
        }
        else if (!type.compare("vel", Qt::CaseInsensitive))
        {
            load->way_ = 2;
        }
        else if (!type.compare("acc", Qt::CaseInsensitive))
        {
            load->way_ = 3;
        }

        if (loadType == "*rload")
        {
            load->way2_ = 0; //rload为0
        }
        else if (loadType == "*rload1")
        {
            load->way2_ = 1; //rload1为1
        }

        double delay = getKeyValue(currentline, "delay", "=").toDouble();
        load->delay_ = delay;

        double xita = getKeyValue(currentline, "xita", "=").toDouble();
        load->phase_ = xita;

        //载荷名称,与两个函数读文件待定,目前往回读无法映射
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_LoadBoundary, getQStringListIndexValue(_linelist, 0));
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, getQStringListIndexValue(_linelist, 1), MapRelation_AmplitudeFunction);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, getQStringListIndexValue(_linelist, 2), MapRelation_PhaseFunction);

        _modeldata->addPhysicalFieldP(load);
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readCombined()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadCombined> load = make_shared<MechCaseData::mLoadCombined>(loadID, LOAD_IM_COMBINED, name);

        double ratio = getKeyValue(currentline, "S", "=").toDouble();
        load->ktotal_ = ratio;

        vector<QString> loadName;
        // 循环读取下一行，直到遇到 * 开头
        while (!_file.atEnd())
        {
            // 读一行并清理空格
            currentline = _file.readLine().simplified();

            // 遇到 * 号，结束读取
            if (currentline.startsWith('*'))
            {
                break;
            }

            _linelist = currentline.split(",");

            // 系数
            load->ratioList_.push_back(getQStringListIndexValue(_linelist, 1).toDouble());

            // 提取载荷名
            loadName.push_back(getQStringListIndexValue(_linelist, 0));
        }
        appendMapPhysicalTable(Physical_LoadBoundary, loadID, Physical_LoadBoundary, loadName);
        _modeldata->addPhysicalFieldP(load);
    }
    void mIMxFile::readAcousticConcForce()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadAcousticConcForce> load = make_shared<MechCaseData::mLoadAcousticConcForce>(loadID, LOAD_IM_ACOCONCFORCE, name);

        QString set = getKeyValue(currentline, "set", "=");
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        if (_linelist.size() > 1)
        {
            //离散
            bool nodeIdValid = false;
            int nodeId = getQStringListIndexValue(_linelist, 0).trimmed().toInt(&nodeIdValid) + _lastMaxNodeID;
            bool valueValid = false;
            double value = getQStringListIndexValue(_linelist, 1).trimmed().toDouble(&valueValid);

            if (nodeIdValid && valueValid)
            {
                load->value_ = value; //存值

                //用节点id创建集合与声学载荷映射
                int setID = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                QString setName = ExistRenamePhysicalName(Physical_Set, QString("set_%1").arg(name));
                MechSetData::mMixSet* eleset = new MechSetData::mMixSet(setID, SetType::Mix, setName, true);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> data;
                data[PickDataType::Node] = { nodeId };
                eleset->setMixSetData(data);
                appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, eleset->getSetName());
            }
        }
        else
        {
            QString value = getQStringListIndexValue(_linelist, 0);
            load->value_ = value.toDouble();
        }
        _modeldata->addPhysicalFieldP(load);
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readAcousticFrequencyDomain()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadAcousticFrequencyDomain> load = make_shared<MechCaseData::mLoadAcousticFrequencyDomain>(loadID, LOAD_IM_ACOFREQDOMAIN, name);

        double delay = getKeyValue(currentline, "delay", "=").toDouble();
        load->delay_ = delay;
        double xita = getKeyValue(currentline, "xita", "=").toDouble();
        load->xita_ = xita;
        double rho = getKeyValue(currentline, "rho", "=").toDouble();
        load->rho_ = rho;
        double b = getKeyValue(currentline, "b", "=").toDouble();
        load->B_ = b;

        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_LoadBoundary, getQStringListIndexValue(_linelist, 0));
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, getQStringListIndexValue(_linelist, 1));
        _modeldata->addPhysicalFieldP(load);
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readBoltload()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);
        QString set = getKeyValue(currentline, "set", "=");
        QString fixed = getKeyValue(currentline, "fixed", "=");
        QString function = getKeyValue(currentline, "function", "=");

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadBolt> load = make_shared<MechCaseData::mLoadBolt>(loadID, LOAD_IM_BOLT, name);
        _modeldata->addPhysicalFieldP(load);

        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);
        if (currentline.startsWith("*nodalload", Qt::CaseInsensitive))
        {
            load->way_ = 0;
            if (set.isEmpty())
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                _boltloadMap[loadID] = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
                load->value_ = getQStringListIndexValue(_linelist, 2);
            }
            else
            {
                set = ExistRenamePhysicalName(Physical_Set, set);
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                load->value_ = getQStringListIndexValue(_linelist, 0);
                _boltloadToSet[loadID] = set;
            }
        }
        else
        {
            if (!fixed.compare("on", Qt::CaseInsensitive))
            {
                load->way_ = 2;
            }
            else
            {
                load->way_ = 1;
            }

            if (set.isEmpty())
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                _boltloadMap[loadID] = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
                load->value_ = getQStringListIndexValue(_linelist, 3);
            }
            else
            {
                set = ExistRenamePhysicalName(Physical_Set, set);
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                load->value_ = getQStringListIndexValue(_linelist, 1);
                _boltloadToSet[loadID] = set;
            }
        }
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readSpectrum()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);
        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadSpectrum> load = make_shared<MechCaseData::mLoadSpectrum>(loadID, LOAD_IM_SPECTRUM, name);
        _modeldata->addPhysicalFieldP(load);
        QString type = getKeyValue(currentline, "type", "=");
        if (!type.compare("D", Qt::CaseInsensitive))
        {
            load->type_ = "位移D";
        }
        else if (!type.compare("V", Qt::CaseInsensitive))
        {
            load->type_ = "速度V";
        }
        else
        {
            load->type_ = "加速度A";
        }
        vector<QString> funcName;
        auto& mapRel = getCVM->getModelData()->getMapRelationManager();
        // 循环读取下一行，直到遇到 * 开头
        int i = 0;
        while (!_file.atEnd())
        {
            // 读一行并清理空格
            currentline = _file.readLine().simplified();

            // 遇到 * 号，结束读取
            if (currentline.startsWith('*'))
            {
                break;
            }

            _linelist = currentline.split(",");
            //阻尼值
            name = getQStringListIndexValue(_linelist, 0);
            load->dampValueList_.push_back(getQStringListIndexValue(_linelist, 1));
            funcName.push_back(name);
        }
        appendMapPhysicalTable(Physical_LoadBoundary, loadID, Physical_Function, funcName);
    }

    void mIMxFile::readPressure()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString type = getKeyValue(currentline, "type", "=");
        QString set = getKeyValue(currentline, "set", "=");
        QString function = getKeyValue(currentline, "function", "=");
        QString file = getKeyValue(currentline, "file", "=");
        QString field = getKeyValue(currentline, "field", "=");

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadPressure> load = make_shared<MechCaseData::mLoadPressure>(loadID, LOAD_IM_PRESSURE, name);

        if (!type.isEmpty() && type.compare("pressure", Qt::CaseInsensitive) != 0)
        {
            // 非 pressure 类型（如 hydrostatic）先按净水压力初始化
            load->pressureType_ = 1;
        }
        else
        {
            load->pressureType_ = 0;
        }

        _modeldata->addPhysicalFieldP(load);

        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Field, field, MapRelationType::MapRelation_AnalysisField);

        if (load->pressureType_ == 1)
        {
            // ========== 净水压力 ==========
            load->way_ = 0;
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            // P, D0, D, V1, V2, V3（单位制换算：文件值(当前界面单位制) → 内部）
            load->refPressure_ = mUnitIoCodec::decode(MxFunctions::Unit::Stress, getQStringListIndexValue(_linelist, 0).trimmed().toDouble());
            load->zeroDepth_ = mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 1).trimmed().toDouble());
            load->refDepth_ = mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 2).trimmed().toDouble());
            load->depthDirection_.setX(getQStringListIndexValue(_linelist, 3).trimmed().toDouble());
            load->depthDirection_.setY(getQStringListIndexValue(_linelist, 4).trimmed().toDouble());
            load->depthDirection_.setZ(getQStringListIndexValue(_linelist, 5).trimmed().toDouble());

            // 同步回填到方向控件 UI 状态：默认使用直接输入模式保存该方向，便于后续反显
            // mDirectionSelectorWidget::DirectionMode 中 DirectInput = 6
            load->depthDirPickState_ = DirectionPickState();
            load->depthDirPickState_.mode = 6;
            load->depthDirPickState_.directInputDir = load->depthDirection_;

            currentline = _file.readLine().simplified();
            return;
        }

        if (!field.isEmpty())
        {
            // ========== 分析场：way_ 保持统一赋值语义，独立标志记录分析场方式，无数据行 ==========
            load->isAnalysisFieldMode_ = true;
            load->way_ = 0;
            currentline = _file.readLine().simplified();
        }
        else if (!file.isEmpty())
        {
            // ========== 文件赋值 ==========
            load->way_ = 2;
            load->path_ = file;

            currentline = _file.readLine().simplified();
        }
        else if (!set.isEmpty())
        {
            // ========== 统一赋值：带set参数 ==========
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);
            load->way_ = 0;

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            //单位制换算：文件值(当前界面单位制) → 内部（压强）
            load->pa_ = mUnitIoCodec::decode(MxFunctions::Unit::Stress, getQStringListIndexValue(_linelist, 0).trimmed().toDouble());
            currentline = _file.readLine().simplified();
        }
        else
        {
            // ========== 独立赋值：不带set参数 ==========
            load->way_ = 1;
            load->cellFaceMap_.clear();

            currentline = _file.readLine().simplified();
            while (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                _linelist = currentline.split(",");
                if (_linelist.size() >= 3)
                {
                    int cellId = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxMeshID;
                    // 解析面ID（去掉开头的B）
                    QString faceStr = getQStringListIndexValue(_linelist, 1).trimmed();
                    if (faceStr.startsWith('B', Qt::CaseInsensitive))
                    {
                        faceStr = faceStr.mid(1);
                    }
                    int faceId = faceStr.toInt();
                    // 解析压强值（单位制换算：文件值(当前界面单位制) → 内部）
                    double pressure = mUnitIoCodec::decode(MxFunctions::Unit::Stress, getQStringListIndexValue(_linelist, 2).trimmed().toDouble());
                    int partID = -1;
                    if (_elementIDToPartID.contains(cellId))
                        partID = _elementIDToPartID[cellId];
                    load->cellFaceMap_[RenderPickResultData(cellId, partID, faceId)] = { pressure };
                }

                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::readHeatFlux()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString set = getKeyValue(currentline, "set", "=");
        QString function = getKeyValue(currentline, "function", "=");
        QString field = getKeyValue(currentline, "field", "=");

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadHeatFlux> load = make_shared<MechCaseData::mLoadHeatFlux>(loadID, LOAD_IM_HEATFLUX, name);
        load->way_ = 1;
        _modeldata->addPhysicalFieldP(load);

        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Field, field, MapRelationType::MapRelation_AnalysisField);

        // ========== 核心：用set参数区分统一/独立赋值 ==========
        if (!set.isEmpty())
        {
            // 统一赋值（原有逻辑完全不变）
            load->defineType_ = 0;
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            if (_linelist.size() >= 1)
            {
                QString value = getQStringListIndexValue(_linelist, 0).trimmed();
                if (!value.isEmpty())
                {
                    load->pa_ = value.toDouble();
                }
            }
            currentline = _file.readLine().simplified();
        }
        else if (!field.isEmpty())
        {
            // 分析场（新增）
            load->defineType_ = 2;
            load->cellFaceData_.clear();
            currentline = _file.readLine().simplified();
        }
        else
        {
            // 独立赋值（新增）
            load->defineType_ = 1;
            load->cellFaceData_.clear();

            currentline = _file.readLine().simplified();
            while (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                _linelist = currentline.split(",");
                if (_linelist.size() >= 3)
                {
                    int cellId = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxMeshID;

                    // 去掉面ID前的B字符
                    QString faceStr = getQStringListIndexValue(_linelist, 1).trimmed();
                    if (faceStr.startsWith('B'))
                    {
                        faceStr = faceStr.mid(1);
                    }
                    int faceId = faceStr.toInt();
                    double val = getQStringListIndexValue(_linelist, 2).trimmed().toDouble();
                    int partID = -1;
                    if (_elementIDToPartID.contains(cellId))
                        partID = _elementIDToPartID[cellId];
                    load->cellFaceData_[RenderPickResultData(cellId, partID, faceId)] = { QVariant(val) };
                }
                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::readRadiation()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString set = getKeyValue(currentline, "set", "=");
        QString function = getKeyValue(currentline, "function", "=");
        QString field = getKeyValue(currentline, "field", "=");

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadRadiation> load = make_shared<MechCaseData::mLoadRadiation>(loadID, LOAD_IM_RADIATION, name);
        _modeldata->addPhysicalFieldP(load);

        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Field, field, MapRelationType::MapRelation_AnalysisField);

        // 用set参数严格区分统一/独立赋值
        if (!set.isEmpty())
        {
            // ========== 统一赋值：原有逻辑完全保留 ==========
            load->defineType_ = 0;
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            if (_linelist.size() >= 2)
            {
                QString value1 = getQStringListIndexValue(_linelist, 0).trimmed();
                load->V2 = value1.toDouble();
                QString value2 = getQStringListIndexValue(_linelist, 1).trimmed();
                load->V1 = value2.toDouble();
            }
            currentline = _file.readLine().simplified();
        }
        else if (!field.isEmpty())
        {
            // ========== 分析场（新增） ==========
            load->defineType_ = 2;
            load->cellFaceData_.clear();
            currentline = _file.readLine().simplified();
        }
        else
        {
            // ========== 独立赋值：新增逻辑 ==========
            load->defineType_ = 1;
            load->cellFaceData_.clear();

            currentline = _file.readLine().simplified();
            // 循环读取所有独立赋值行，直到遇到下一个*命令
            while (!currentline.isEmpty() && !currentline.startsWith("*"))
            {
                _linelist = currentline.split(",");
                if (_linelist.size() >= 4)
                {
                    int eid = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxMeshID;

                    // 解析面ID，自动去掉前缀B
                    QString faceStr = getQStringListIndexValue(_linelist, 1).trimmed();
                    if (faceStr.startsWith('B', Qt::CaseInsensitive))
                    {
                        faceStr = faceStr.mid(1);
                    }
                    int fid = faceStr.toInt();

                    // 解析辐射系数e和环境温度t
                    double eVal = getQStringListIndexValue(_linelist, 2).trimmed().toDouble();
                    double tVal = getQStringListIndexValue(_linelist, 3).trimmed().toDouble();
                    int partID = -1;
                    if (_elementIDToPartID.contains(eid))
                        partID = _elementIDToPartID[eid];
                    load->cellFaceData_[RenderPickResultData(eid, partID, fid)] = { QVariant(tVal), QVariant(eVal) };
                }
                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::readConvection()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString set = getKeyValue(currentline, "set", "=");
        QString file = getKeyValue(currentline, "file", "=");
        QString function = getKeyValue(currentline, "function", "=");
        QString functionConv = getKeyValue(currentline, "convfunction", "=");
        QString field = getKeyValue(currentline, "field", "=");
        QString convField = getKeyValue(currentline, "convfield", "=");

        int loadID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadConvection> load = make_shared<MechCaseData::mLoadConvection>(loadID, LOAD_IM_CONVECTION, name);
        _modeldata->addPhysicalFieldP(load);

        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, function);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Function, functionConv, MapRelationType::MapRelation_convFunction);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Field, field, MapRelationType::MapRelation_AnalysisField);
        appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Field, convField, MapRelationType::MapRelation_convAnalysisField);

        if (!file.isEmpty())
        {
            // ========== 文件定义 ==========
            load->defineType_ = 2;

            //集合映射
            appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

            //读文件路径
            QFileInfo fi(_filename);
            QString dirPath = fi.absolutePath();
            load->path_ = dirPath + "/" + file;

            //读数字
            currentline = _file.readLine().simplified();
            if (!currentline.isEmpty())
            {
                _linelist = currentline.split(",");
                load->V1 = getQStringListIndexValue(_linelist, 0);
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }
        else if (!field.isEmpty() || !convField.isEmpty())
        {
            // ========== 分析场（新增） ==========
            load->defineType_ = 3;
            load->cellFaceData_.clear();

            //集合映射
            if (!set.isEmpty())
            {
                appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);
            }
            currentline = _file.readLine().simplified();
        }
        else
        {
            if (!set.isEmpty())
            {
                // ========== 统一赋值：原有逻辑增强，支持空值 ==========
                load->defineType_ = 0;
                appendMapPhysicalStruct(Physical_LoadBoundary, loadID, Physical_Set, set);

                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                if (_linelist.size() >= 2)
                {
                    QString hStr = getQStringListIndexValue(_linelist, 0).trimmed();
                    load->V2 = hStr.isEmpty() ? QVariant() : QVariant(hStr.toDouble());

                    QString tStr = getQStringListIndexValue(_linelist, 1).trimmed();
                    load->V1 = tStr.isEmpty() ? QVariant() : QVariant(tStr.toDouble());
                }
                currentline = _file.readLine().simplified();
            }
            else
            {
                load->defineType_ = 1;
                load->cellFaceData_.clear();

                currentline = _file.readLine().simplified();
                // 循环读取所有独立赋值行，直到遇到下一个*命令
                while (!currentline.isEmpty() && !currentline.startsWith("*"))
                {
                    _linelist = currentline.split(",");
                    if (_linelist.size() >= 4)
                    {
                        int eid = getQStringListIndexValue(_linelist, 0).trimmed().toInt() + _lastMaxMeshID;

                        QString faceStr = getQStringListIndexValue(_linelist, 1).trimmed();
                        if (faceStr.startsWith('B'))
                        {
                            faceStr = faceStr.mid(1);
                        }
                        int fid = faceStr.toInt();

                        QString hStr = getQStringListIndexValue(_linelist, 2).trimmed();
                        QVariant hVal = hStr.isEmpty() ? QVariant() : QVariant(hStr.toDouble());

                        QString tStr = getQStringListIndexValue(_linelist, 3).trimmed();
                        QVariant tVal = tStr.isEmpty() ? QVariant() : QVariant(tStr.toDouble());

                        int partID = -1;
                        if (_elementIDToPartID.contains(eid))
                            partID = _elementIDToPartID[eid];
                        // 存入格式：RenderPickResultData -> <环境温度t, 对流换热系数h>
                        load->cellFaceData_[RenderPickResultData(eid, partID, fid)] = { tVal, hVal };
                    }
                    currentline = _file.readLine().simplified();
                }
            }
        }
    }
    void mIMxFile::readBoundary()
    {
        if (currentline.startsWith("*spc", Qt::CaseInsensitive))
        {
            readSpc();
        }
        else if (currentline.startsWith("*suport1", Qt::CaseInsensitive))
        {
            readSuport();
        }
        else if (currentline.startsWith("*constraint", Qt::CaseInsensitive))
        {
            readSuperelement();
        }
        else
        {
            currentline = _file.readLine().simplified();
        }
    }
    void mIMxFile::readCoordinatesystem()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_Coordinate, name);
        QString type = getKeyValue(currentline, "type", "=");
        QString ttype = getKeyValue(currentline, "ttype", "=");

        //法向 必存在
        int normalDir = getKeyValue(currentline, "normaldirection", "=").toInt();
        //离散转角关键字（有就是离散，没有就是无/固定）
        bool hasRotationField = currentline.contains("rotationfield", Qt::CaseInsensitive);
        QString rotField = getKeyValue(currentline, "rotationfield", "=");
        //固定转角
        bool hasTheta = currentline.contains("theta", Qt::CaseInsensitive);
        QString thetaStr = getKeyValue(currentline, "theta", "=");
        //离散场名称（离散坐标系用）
        QString distributionName = getKeyValue(currentline, "distribution", "=");

        int coorID = _modeldata->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).createID();
        MechCoordinateData::mCoordinateBase* coor = new MechCoordinateData::mCoordinateBase(coorID, Cartesian, name);
        _modeldata->addPhysicalField<MechCoordinateData::mCoordinateBase>(coor);
        coor->setMethod("三点");
        //2.存法向
        coor->_normaldirection = normalDir;
        // 3. 判断转角类型
        if (hasRotationField)
        {
            // 离散转角
            coor->_rotAngleType = 2;
            coor->_rotFieldName = rotField;
            coor->_rotAngle = "";
        }
        else if (hasTheta)
        {
            // 固定转角
            coor->_rotAngleType = 1;
            coor->_rotAngle = thetaStr;
            coor->_rotFieldName = "";
        }
        else
        {
            // 先默认：无转角
            coor->_rotAngleType = 0;
            coor->_rotFieldName = "";
            coor->_rotAngle = "";
        }
        if (!ttype.compare("xoy", Qt::CaseInsensitive))
        {
            coor->setMainIndex(0);
            coor->setDirIndex(0);
        }
        else
        {
            coor->setMainIndex(2);
            coor->setDirIndex(0);
        }

        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");

        auto setXYZ = [&]() {
            QVector3D vec3D;
            //单位制换算：文件值(当前界面单位制) → 内部mm-t-N-s（坐标点为长度）
            vec3D = QVector3D(mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 0).toDouble()),
                mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 1).toDouble()),
                mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 2).toDouble()));
            coor->setOrigin(vec3D);
            vec3D = QVector3D(mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 3).toDouble()),
                mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 4).toDouble()),
                mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 5).toDouble()));
            coor->setXPoint(vec3D);
            vec3D = QVector3D(mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 6).toDouble()),
                mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 7).toDouble()),
                mUnitIoCodec::decode(MxFunctions::Unit::Length, getQStringListIndexValue(_linelist, 8).toDouble()));
            coor->setXYPoint(vec3D);
            };

        auto setNodeMap = [&, this]() {
            int node; vector<int> coorNodes;
            coorNodes.reserve(3);
            node = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            coorNodes.push_back(node);
            node = getQStringListIndexValue(_linelist, 1).toInt() + _lastMaxNodeID;
            coorNodes.push_back(node);
            node = getQStringListIndexValue(_linelist, 2).toInt() + _lastMaxNodeID;
            coorNodes.push_back(node);
            _coorToNodes[coorID] = coorNodes;
            };

        //5. 判断是否为固定转角
        // 没有rotationfield + 第二行有10个值 → 固定转角
        if (rotField.isEmpty() && _linelist.size() >= 10)
        {
            coor->_rotAngleType = 1;
            coor->_rotAngle = getQStringListIndexValue(_linelist, 9).simplified();
        }

        if (!type.compare("rectangular", Qt::CaseInsensitive))
        {
            coor->setCoordinateType(Cartesian);
            setXYZ();
        }
        else if (!type.compare("rectangular_node", Qt::CaseInsensitive))
        {
            coor->setCoordinateType(Cartesian);
            setNodeMap();
        }
        else if (!type.compare("cylindrical", Qt::CaseInsensitive))
        {
            coor->setCoordinateType(Cylindrical);
            setXYZ();
        }
        else if (!type.compare("cylindrical_node", Qt::CaseInsensitive))
        {
            coor->setCoordinateType(Cylindrical);
            setNodeMap();
        }
        else if (!type.compare("spherical", Qt::CaseInsensitive))
        {
            coor->setCoordinateType(Spherical);
            setXYZ();
        }
        else if (!type.compare("spherical_node", Qt::CaseInsensitive))
        {
            coor->setCoordinateType(Spherical);
            setNodeMap();
        }
        else if (!type.compare("distribution", Qt::CaseInsensitive))
        {
            coor->setCoordinateType(DiscreteCoordinate);

            // 建立坐标系与离散场的映射关系
            if (!distributionName.isEmpty())
            {
                appendMapPhysicalStruct(Physical_Coordinate, coorID, Physical_Field, distributionName);
            }

            // 离散坐标系只有一行，没有轴数据行
            // 当前 currentline 已在前面被读为下一关键字行，直接返回
            return;
        }
        else
        {
            currentline = _file.readLine().simplified();
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("坐标系类型 %1 暂不支持！").arg(type));
        }

        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readSpc()
    {

        QString type = getKeyValue(currentline, "type", "=").trimmed();
        MxFunctions::LoadBoundaryType typeEnum;
        if (type == "submodeldisp")
        {
            typeEnum = BOUNDARY_MOVE;
        }
        else if (type == "submodeltemp")
        {
            typeEnum = BOUNDARY_TEMP_NEW;
        }
        else if (type == "velocity")
        {
            typeEnum = BOUNDARY_IM_SPEED;
        }
        else if (type == "acceleration")
        {
            typeEnum = BOUNDARY_IM_ACC;
        }
        else
        {
            typeEnum = BOUNDARY_IM_DISPLACEMENT;
            QString boltload = getKeyValue(currentline, "boltload", "=");
            if (!boltload.compare("on", Qt::CaseInsensitive))
            {
                readBoltload();
                return;
            }
        }

        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString set = getKeyValue(currentline, "set", "=");
        QString function = getKeyValue(currentline, "function", "=");
        QString field = getKeyValue(currentline, "field", "=");

        int bcID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        QHash<QString, int> hash_type = {
            {"encastre", 0}, {"pinned", 1},
            {"xsymm", 2}, {"ysymm", 3},{"zsymm", 4},
            {"xasymm", 5}, {"yasymm", 6},{"zasymm", 7}
        };


        currentline = _file.readLine().simplified();
        // 温度边界分析场：有 field 关键字且无 set/数据行，直接创建 mBoundaryTemp
        if (!field.isEmpty() && set.isEmpty())
        {
            shared_ptr<MechCaseData::mBoundaryTemp> bc = make_shared<MechCaseData::mBoundaryTemp>(bcID, BOUNDARY_IM_TEMP, name);
            _modeldata->addPhysicalFieldP(bc);
            bc->isAnalysisFieldMode_ = true;
            appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Function, function);
            appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Field, field, MapRelationType::MapRelation_AnalysisField);
            return;
        }
        if (!currentline.compare("encastre", Qt::CaseInsensitive) || !currentline.compare("pinned", Qt::CaseInsensitive)
            || !currentline.compare("xsymm", Qt::CaseInsensitive) || !currentline.compare("ysymm", Qt::CaseInsensitive) || !currentline.compare("zsymm", Qt::CaseInsensitive)
            || !currentline.compare("xasymm", Qt::CaseInsensitive) || !currentline.compare("yasymm", Qt::CaseInsensitive) || !currentline.compare("zasymm", Qt::CaseInsensitive))
        {
            shared_ptr<MechCaseData::mBoundaryFixed> bc = make_shared<MechCaseData::mBoundaryFixed>(bcID, BOUNDARY_IM_FIXED, name);
            _modeldata->addPhysicalFieldP(bc);
            bc->_fixedType = hash_type[currentline];
            appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Function, function);
            appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Set, set);
            currentline = _file.readLine().simplified();
        }

        else if (currentline.startsWith("*mastermodel", Qt::CaseInsensitive))
        {
            QString mxdb = getKeyValue(currentline, "type", "=");
            QString fileName = getKeyValue(currentline, "file", "=");
            QString stepName = getKeyValue(currentline, "step", "=");
            QString setName = getKeyValue(currentline, "set", "=");
            if (typeEnum == BOUNDARY_MOVE)
            {
                readSubmodelMove(name, set);
            }
            else if (typeEnum == BOUNDARY_TEMP_NEW)
            {
                readSubmodelTempNew(name, set);
            }
        }
        else
        {
            // 4 列 -> 独立赋值；2 列 -> 各向分量/温度
            bool isIndependentMode = false;
            bool hasTemp = false;
            unordered_map<int, double> dof_value;
            std::map<RenderPickResultData, QVector<QVariant>> independentTable;

            QString firstDataLine = currentline;
            while (!firstDataLine.isEmpty() && !_file.atEnd())
            {
                if (firstDataLine.startsWith("*"))
                    break;
                QStringList firstList = firstDataLine.split(",");
                if (firstList.size() >= 4)
                {
                    isIndependentMode = true;
                    break;
                }
                else if (firstList.size() >= 2)
                {
                    isIndependentMode = false;
                    break;
                }
                firstDataLine = _file.readLine().simplified();
            }

            while (!_file.atEnd())
            {
                if (currentline.startsWith("*"))
                    break;

                if (currentline.isEmpty())
                {
                    currentline = _file.readLine().simplified();
                    continue;
                }

                _linelist = currentline.split(",");
                if (isIndependentMode && _linelist.size() >= 4)
                {
                    // 独立赋值格式：nodeId, startDof, endDof, value
                    int nodeId = getQStringListIndexValue(_linelist, 0).trimmed().toInt();
                    int startDof = getQStringListIndexValue(_linelist, 1).trimmed().toInt();
                    int endDof = getQStringListIndexValue(_linelist, 2).trimmed().toInt();
                    double val = getQStringListIndexValue(_linelist, 3).trimmed().toDouble();

                    RenderPickResultData key(nodeId, -1, -1);
                    QVector<QVariant>& values = independentTable[key];
                    if (values.size() < 6)
                        values.resize(6);
                    for (int dof = startDof; dof <= endDof; ++dof)
                    {
                        if (dof >= 1 && dof <= 6)
                        {
                            values[dof - 1] = val;
                        }
                    }
                }
                else if (!isIndependentMode && _linelist.size() >= 2)
                {
                    // 各向分量/温度：第一列为 dof，第二列为 value
                    int key = getQStringListIndexValue(_linelist, 0).trimmed().toInt();
                    double val = getQStringListIndexValue(_linelist, 1).trimmed().toDouble();
                    if (key >= 1 && key <= 7)
                    {
                        dof_value[key] = val;
                    }
                }

                currentline = _file.readLine().simplified();
            }

            if (!isIndependentMode && dof_value.find(7) != dof_value.cend())
            {
                shared_ptr<MechCaseData::mBoundaryTemp> bc = make_shared<MechCaseData::mBoundaryTemp>(bcID, BOUNDARY_IM_TEMP, name);
                _modeldata->addPhysicalFieldP(bc);
                appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Function, function);
                appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Set, set);
                bc->_temp = dof_value[7];
                dof_value.erase(7);
            }

            if (!isIndependentMode && dof_value.size() > 0)
            {
                shared_ptr<MechCaseData::mBoundaryDisplacement> bc = make_shared<MechCaseData::mBoundaryDisplacement>(bcID, typeEnum, name);
                _modeldata->addPhysicalFieldP(bc);
                appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Function, function);
                appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Set, set);
                // 各向分量模式
                bc->_definitionMode = 0;

                for (auto& it : dof_value)
                {
                    int dof = it.first;
                    double val = it.second;
                    switch (dof)
                    {
                    case 1:
                        bc->_U1 = val;
                        break;
                    case 2:
                        bc->_U2 = val;
                        break;
                    case 3:
                        bc->_U3 = val;
                        break;
                    case 4:
                        bc->_UR1 = val;
                        break;
                    case 5:
                        bc->_UR2 = val;
                        break;
                    case 6:
                        bc->_UR3 = val;
                        break;
                    default:
                        break;
                    }
                }
            }
            else if (isIndependentMode && !independentTable.empty())
            {
                shared_ptr<MechCaseData::mBoundaryDisplacement> bc = make_shared<MechCaseData::mBoundaryDisplacement>(bcID, typeEnum, name);
                _modeldata->addPhysicalFieldP(bc);
                appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Function, function);
                // 独立赋值模式不写 set，不建立 set 映射
                bc->_definitionMode = 1;
                bc->_independentTable = independentTable;
            }
        }
    }

    void mIMxFile::readSubmodelMove(const QString& name, const QString& set)
    {
        QString filePath = getKeyValue(currentline, "file", "=");
        QString timeStep = getKeyValue(currentline, "step", "=");
        QString mainSet = getKeyValue(currentline, "set", "=");

        int bcID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mBoundaryMove> bc = make_shared<MechCaseData::mBoundaryMove>(bcID, BOUNDARY_MOVE, name);
        _modeldata->addPhysicalFieldP(bc);


        if (!filePath.isEmpty())
        {
            if (QFileInfo(filePath).isAbsolute())
            {
                bc->_mainModelFile = filePath;
            }
            else
            {
                QFileInfo fi(_filename);
                bc->_mainModelFile = fi.absolutePath() + "/" + filePath;
            }
        }
        else
        {
            bc->_mainModelFile = filePath;
        }
        bc->_timeStep = timeStep;
        bc->_mainSet = mainSet;
        bc->_useMainSet = !mainSet.isEmpty();

        appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Set, set);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readSubmodelTempNew(const QString& name, const QString& set)
    {
        QString filePath = getKeyValue(currentline, "file", "=");
        QString timeStep = getKeyValue(currentline, "step", "=");
        QString mainSet = getKeyValue(currentline, "set", "=");

        int bcID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mBoundaryTempNew> bc = make_shared<MechCaseData::mBoundaryTempNew>(bcID, BOUNDARY_TEMP_NEW, name);
        _modeldata->addPhysicalFieldP(bc);


        if (!filePath.isEmpty())
        {
            if (QFileInfo(filePath).isAbsolute())
            {
                bc->_mainModelFile = filePath;
            }
            else
            {
                QFileInfo fi(_filename);
                bc->_mainModelFile = fi.absolutePath() + "/" + filePath;
            }
        }
        else
        {
            bc->_mainModelFile = filePath;
        }
        bc->_timeStep = timeStep;
        bc->_mainSet = mainSet;
        bc->_useMainSet = !mainSet.isEmpty();

        appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Set, set);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readSuport()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString set = getKeyValue(currentline, "set", "=");
        int bcID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();

        shared_ptr<MechCaseData::mBoundaryVirtual> bc = make_shared<MechCaseData::mBoundaryVirtual>(bcID, BOUNDARY_IM_VIRTUAL, name);
        _modeldata->addPhysicalFieldP(bc);
        appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Set, set);
        currentline = _file.readLine().simplified();

        while (!_file.atEnd())
        {
            if (currentline.startsWith("*"))
                break;

            if (currentline.isEmpty())
            {
                currentline = _file.readLine().simplified();
                continue;
            }

            if (!currentline.compare("1"))
            {
                bc->_U1 = true;
            }
            else if (!currentline.compare("2"))
            {
                bc->_U2 = true;
            }
            else if (!currentline.compare("3"))
            {
                bc->_U3 = true;
            }
            else if (!currentline.compare("4"))
            {
                bc->_UR1 = true;
            }
            else if (!currentline.compare("5"))
            {
                bc->_UR2 = true;
            }
            else if (!currentline.compare("6"))
            {
                bc->_UR3 = true;
            }

            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readSuperelement()
    {
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_LoadBoundary, name);

        QString type = getKeyValue(currentline, "type", "=");

        QString set = getKeyValue(currentline, "set", "=");
        int bcID = _modeldata->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).createID();

        shared_ptr<MechCaseData::mBoundarySuperelement> bc = make_shared<MechCaseData::mBoundarySuperelement>(bcID, BOUNDARY_IM_SUPERELEMENT, name);
        _modeldata->addPhysicalFieldP(bc);
        appendMapPhysicalStruct(Physical_LoadBoundary, bcID, Physical_Set, set);
        bc->_ConstraintType = type;

        currentline = _file.readLine().simplified();

        while (!_file.atEnd())
        {
            if (currentline.startsWith("*"))
                break;

            if (currentline.isEmpty())
            {
                currentline = _file.readLine().simplified();
                continue;
            }

            if (!currentline.compare("1"))
            {
                bc->_DOF[0] = true;
            }
            else if (!currentline.compare("2"))
            {
                bc->_DOF[1] = true;
            }
            else if (!currentline.compare("3"))
            {
                bc->_DOF[2] = true;
            }
            else if (!currentline.compare("4"))
            {
                bc->_DOF[3] = true;
            }
            else if (!currentline.compare("5"))
            {
                bc->_DOF[4] = true;
            }
            else if (!currentline.compare("6"))
            {
                bc->_DOF[5] = true;
            }
            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readConnector()
    {
        if (currentline.startsWith("*connector_part", Qt::CaseInsensitive))
        {
            currentline = _file.readLine().simplified();
        }

        do
        {
            setProgressBarValue(_file.pos());
            QString type = getKeyValue(currentline, "type", "=");
            if (!type.compare("rigid", Qt::CaseInsensitive))
            {
                readRigidConnector();
            }
            else if (!type.compare("flexible", Qt::CaseInsensitive))
            {
                readFlexibleConnector();
            }
            else if (!type.compare("tie", Qt::CaseInsensitive) || !type.compare("n2stie", Qt::CaseInsensitive) || !type.compare("s2stie", Qt::CaseInsensitive))
            {
                readBind();
            }
            else if (!type.compare("mpc", Qt::CaseInsensitive))
            {
                readMPC();
            }
            else if (!type.compare("fastener", Qt::CaseInsensitive))
            {
                readFastener();
            }
            else if (!type.compare("distributing", Qt::CaseInsensitive))
            {
                readDistributedCoupling();
            }
            else
            {
                currentline = _file.readLine().simplified();
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("连接类型 %1 暂不支持！").arg(type));
            }
        } while (!_file.atEnd() && currentline.startsWith("*connector", Qt::CaseInsensitive) && !currentline.startsWith("*connector_part", Qt::CaseInsensitive));
    }
    void mIMxFile::readRigidConnector()
    {
        int connectID = _modeldata->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).createID();
        shared_ptr<MechConnectorData::mConnectorRigid> connect = make_shared<MechConnectorData::mConnectorRigid>(connectID, Connector_Rigid);
        connect->degreeoffreedom_main.clear();
        connect->way_ = 1;
        _modeldata->addPhysicalFieldP(connect);

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*masternodes", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                QString str_node = getQStringListIndexValue(_linelist, 0);
                if (!str_node.isEmpty())
                {
                    int node = str_node.toInt() + _lastMaxNodeID;
                    connect->mainnodes_ = { PickDataType::Node, node };
                }
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*slavenodes", Qt::CaseInsensitive))
            {
                while (!_file.atEnd())
                {
                    currentline = _file.readLine().simplified();
                    if (currentline.startsWith("*", Qt::CaseInsensitive))
                        break;
                    if (currentline.isEmpty())
                        continue;

                    _linelist = currentline.split(",");
                    for (int i = 0; i < _linelist.size(); i++)
                    {
                        QString str_node = getQStringListIndexValue(_linelist, i);
                        if (!str_node.isEmpty())
                        {
                            int node = str_node.toInt() + _lastMaxNodeID;
                            connect->subnodes_[PickDataType::Node].push_back(node);
                        }
                    }
                }
            }
            else if (currentline.startsWith("*slavedofs", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                for (int i = 0; i < _linelist.size(); i++)
                {
                    QString dof = getQStringListIndexValue(_linelist, i);
                    if (!dof.isEmpty())
                    {
                        connect->degreeoffreedom_main.insert(dof.toInt());
                    }
                }
                currentline = _file.readLine().simplified();
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }

    }
    void mIMxFile::readFlexibleConnector()
    {
        int connectID = _modeldata->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).createID();
        shared_ptr<MechConnectorData::mConnectorFlexible> connect = make_shared<MechConnectorData::mConnectorFlexible>(connectID, Connector_Flexible);
        connect->way_ = 1;
        connect->degreeoffreedom_sub.clear();
        _modeldata->addPhysicalFieldP(connect);

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*masternodes", Qt::CaseInsensitive))
            {
                while (!_file.atEnd())
                {
                    currentline = _file.readLine().simplified();
                    if (currentline.startsWith("*", Qt::CaseInsensitive))
                        break;
                    if (currentline.isEmpty())
                        continue;

                    _linelist = currentline.split(",");
                    QString str_node = getQStringListIndexValue(_linelist, 0);
                    int node = str_node.toInt() + _lastMaxNodeID;
                    connect->mainnodes_[PickDataType::Node].push_back(node);
                    QString weight_ = getQStringListIndexValue(_linelist, 1);
                    connect->w_.push_back(weight_.toDouble());

                    set<int> dof;
                    for (int i = 2; i < _linelist.size(); i++)
                    {
                        QString str_dof = getQStringListIndexValue(_linelist, i);
                        if (!str_dof.isEmpty())
                        {
                            dof.insert(str_dof.toInt());
                        }
                    }
                    connect->dof_.push_back(dof);
                    // 读文件统一回表格，默认使用外部模式展示
                    // 不设置 isTableMode_，由界面决定
                }
            }
            else if (currentline.startsWith("*slavenodes", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                QString str_node = getQStringListIndexValue(_linelist, 0);
                if (!str_node.isEmpty())
                {
                    int node = str_node.toInt() + _lastMaxNodeID;
                    connect->subnodes_ = { PickDataType::Node, node };
                }
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*slavedofs", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                for (int i = 0; i < _linelist.size(); i++)
                {
                    QString dof = getQStringListIndexValue(_linelist, i);
                    if (!dof.isEmpty())
                    {
                        connect->degreeoffreedom_sub.insert(dof.toInt());
                    }
                }
                currentline = _file.readLine().simplified();
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }


    }
    void mIMxFile::readBind()
    {
        QString type = getKeyValue(currentline, "type", "=");
        QString adjust = getKeyValue(currentline, "adjust", "=");
        int connectID = _modeldata->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).createID();
        shared_ptr<MechConnectorData::mConnectorBind> connect = make_shared<MechConnectorData::mConnectorBind>(connectID, Connector_Bind);
        _modeldata->addPhysicalFieldP(connect);

        if (!type.compare("s2stie", Qt::CaseInsensitive))
        {
            connect->type_ = 1;
        }
        else
        {
            connect->type_ = 0;
        }

        if (!adjust.isEmpty())
        {
            if (!adjust.compare("on", Qt::CaseInsensitive))
            {
                connect->adjust_ = true;
            }
            else if (!adjust.compare("off", Qt::CaseInsensitive))
            {
                connect->adjust_ = false;
            }
        }

        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        QString slaverName = getQStringListIndexValue(_linelist, 0);
        QString masterName = getQStringListIndexValue(_linelist, 1);
        appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Set, slaverName, MapRelation_Slave);
        appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Set, masterName, MapRelation_Master);

        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*tol", Qt::CaseInsensitive))
        {
            type = getKeyValue(currentline, "type", "=");
            connect->allowrancetype_ = type.toInt();
            if (connect->allowrancetype_ == 0)
            {
                currentline = _file.readLine().simplified();
                connect->allowrance_ = currentline.toDouble();
            }
            currentline = _file.readLine().simplified();
        }

    }
    void mIMxFile::readMPC()
    {
        int connectID = _modeldata->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).createID();
        shared_ptr<MechConnectorData::mConnectorMPC> connect = make_shared<MechConnectorData::mConnectorMPC>(connectID, Connector_MPC);
        _modeldata->addPhysicalFieldP(connect);

        currentline = _file.readLine().simplified();
        if (!currentline.contains(","))
        {
            connect->coefficient_ = currentline.toDouble();
            currentline = _file.readLine().simplified();
        }

        bool salveNode = true;
        int nodeID, tempID, dof, weight;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*"))break;
            _linelist = currentline.split(",");
            if (_linelist.size() >= 3)
            {
                if (salveNode)
                {
                    RenderPickResultData data;
                    data.result_id = _lastMaxNodeID + getQStringListIndexValue(_linelist, 0).toInt();
                    connect->subnodes_ = { PickDataType::Node, data };
                    connect->subDof_ = getQStringListIndexValue(_linelist, 1).toInt() - 1;
                    connect->subWeights_ = getQStringListIndexValue(_linelist, 2).toDouble();
                    salveNode = false;
                }
                else
                {
                    nodeID = _lastMaxNodeID + getQStringListIndexValue(_linelist, 0).toInt();
                    appendRenderPickResultData(connect->mainnodes_[PickDataType::Node], nodeID);
                    vector<QVariant> main_var(6);
                    dof = getQStringListIndexValue(_linelist, 1).toInt();
                    weight = getQStringListIndexValue(_linelist, 2).toDouble();
                    main_var[dof - 1] = weight;

                    while (!_file.atEnd())
                    {
                        currentline = _file.readLine().simplified();
                        _linelist = currentline.split(",");
                        if (currentline.startsWith("*"))break;
                        tempID = _lastMaxNodeID + getQStringListIndexValue(_linelist, 0).toInt();
                        if (nodeID != tempID)break;
                        dof = getQStringListIndexValue(_linelist, 1).toInt();
                        weight = getQStringListIndexValue(_linelist, 2).toDouble();
                        main_var[dof - 1] = weight;
                    }

                    connect->mainweights_.push_back(main_var);
                    continue;
                }
            }
            currentline = _file.readLine().simplified();
        }

    }
    void mIMxFile::readDisperse()
    {
        int connectID = _modeldata->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).createID();
        shared_ptr<MechConnectorData::mConnectorDisperse> connect = make_shared<MechConnectorData::mConnectorDisperse>(connectID, Connector_Disperse);
        _modeldata->addPhysicalFieldP(connect);

        if (currentline.startsWith("*rigidbody", Qt::CaseInsensitive))
        {
            QString elementSet = getKeyValue(currentline, "set", "=");//取出单元集名
            QString elementSet1 = getKeyValue(currentline, "analyticalsurface", "=");//取出单元集名
            if (elementSet != "" )
            {
                appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Set, elementSet, MapRelation_Master);
            }
            else if (elementSet1 != "")
            {
                appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Set, elementSet1, MapRelation_Master);
            }

            // 节点集名：原始行中 refnode 带引号表示集合名，不带引号表示节点ID
            QRegularExpression refRe("refnode\\s*=\\s*\"([^\"]*)\"", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch refMatch = refRe.match(currentline);
            if (refMatch.hasMatch())
            {
                appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Set, refMatch.captured(1), MapRelation_Slave);
            }
            else
            {
                int nodeId = getKeyValue(currentline, "refnode", "=").toInt() + _lastMaxNodeID;

                QString name = ExistRenamePhysicalName(Physical_Set, "contain");
                MechSetData::mMixSet* set = new MechSetData::mMixSet(nodeId, SetType::Mix, name, true);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
                appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Set, set->getSetName(), MapRelation_Slave);
            }
        }

        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readDistributedCoupling()
    {
        int connectID = _modeldata->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).createID();
        shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connect = make_shared<MechConnectorData::mConnectorDistributedCoupling>(connectID, Connector_DistributedCoupling);
        connect->way_ = 1;
        _modeldata->addPhysicalFieldP(connect);

        // 读取主节点
        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*masternodes", Qt::CaseInsensitive))
        {
            currentline = _file.readLine().simplified();
            while (!_file.atEnd())
            {
                if (currentline.startsWith("*")) break;
                _linelist = currentline.split(",");
                if (_linelist.size() >= 2)
                {
                    int nodeID = _lastMaxNodeID + getQStringListIndexValue(_linelist, 0).toInt();
                    double weight = getQStringListIndexValue(_linelist, 1).toDouble();
                    appendRenderPickResultData(connect->mainnodes_[PickDataType::Node], nodeID);
                    connect->w_.push_back(weight);
                }
                currentline = _file.readLine().simplified();
            }
        }

        // 读取从节点
        if (currentline.startsWith("*slavenodes", Qt::CaseInsensitive))
        {
            currentline = _file.readLine().simplified();
            if (!currentline.startsWith("*"))
            {
                int nodeID = _lastMaxNodeID + currentline.toInt();
                RenderPickResultData data;
                data.result_id = nodeID;
                connect->subnodes_ = { PickDataType::Node, data };
                currentline = _file.readLine().simplified();
            }
        }

        // 读取从节点自由度
        if (currentline.startsWith("*slavedofs", Qt::CaseInsensitive))
        {
            currentline = _file.readLine().simplified();
            while (!_file.atEnd())
            {
                if (currentline.startsWith("*")) break;
                _linelist = currentline.split(",");
                for (int i = 0; i < _linelist.size(); i++)
                {
                    QString str_dof = getQStringListIndexValue(_linelist, i);
                    if (!str_dof.isEmpty())
                    {
                        connect->degreeoffreedom_main.insert(str_dof.toInt());
                    }
                }
                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::readPretension()
    {
        QString node = getKeyValue(currentline, "node");
        QString element = getKeyValue(currentline, "element");
        QString set = getKeyValue(currentline, "set", "=");

        int connectID = _modeldata->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).createID();
        shared_ptr<MechConnectorData::mConnectorBolt> connect = make_shared<MechConnectorData::mConnectorBolt>(connectID, Connector_Bolt);
        _modeldata->addPhysicalFieldP(connect);

        if (node.contains("\""))
        {
            node = getKeyValue(currentline, "node", "=");
            node = ExistRenamePhysicalName(Physical_Set, node);
            _SetToPretension[node] = connectID;
            node = ExistRenamePhysicalName(Physical_Connector, node);
            connect->setConnectorName(node);
        }
        else
        {
            QString name = _modeldata->createNameByType(Physical_Connector, connect->getConnectorTypeToStringEn());
            name = ExistRenamePhysicalName(Physical_Connector, name);
            connect->setConnectorName(name);
            node = getKeyValue(currentline, "node", "=");
            int nid = node.toInt() + _lastMaxNodeID;
            connect->node_ = nid;
            _pretensionMap[nid] = connectID;
        }

        if (set.isEmpty()) //一维
        {
            connect->type_ = 0;
            if (element.contains("\""))
            {
                element = getKeyValue(currentline, "element", "=");
            }
            else
            {
                element = getKeyValue(currentline, "element", "=");
                RenderPickResultData data;
                data.result_id = element.toInt() + _lastMaxMeshID;

                int setID = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                element = ExistRenamePhysicalName(Physical_Set, "element_pretension");
                MechSetData::mMixSet* set = new MechSetData::mMixSet(setID, Mix, element, true, true);
                _modeldata->addPhysicalField<MechSetData::mSetBase>(set);

                unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> datas;
                vector<RenderPickResultData> ids;
                ids.push_back(data);
                datas[MxFunctions::PickDataType::Element] = ids;
                set->setMixSetData(datas);
            }
            appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Set, element);
        }
        else
        {
            connect->type_ = 1;
            appendMapPhysicalStruct(Physical_Connector, connectID, Physical_Set, set);
        }

        currentline = _file.readLine().simplified();
        if (!currentline.isEmpty() && !currentline.startsWith("*"))
        {
            _linelist = currentline.split(",");
            connect->isOpen_ = true;
            connect->way_ = 1;
            connect->x_ = getQStringListIndexValue(_linelist, 0).toDouble();
            connect->y_ = getQStringListIndexValue(_linelist, 1).toDouble();
            connect->z_ = getQStringListIndexValue(_linelist, 2).toDouble();
            currentline = _file.readLine().simplified();
        }

    }
    void mIMxFile::boltloadMap()
    {
        MechMapRelation::mMapRelationManager& manager = _modeldata->getMapRelationManager();
        //螺栓载荷
        for (auto it = _boltloadMap.begin(); it != _boltloadMap.cend(); ++it)
        {
            if (_pretensionMap.constFind(it.value()) != _pretensionMap.cend())
            {
                manager.appendMapRelation({ Physical_LoadBoundary, it.key() }, { Physical_Connector, _pretensionMap[it.value()] });
            }
        }
        for (auto it = _boltloadToSet.begin(); it != _boltloadToSet.cend(); ++it)
        {
            if (_SetToPretension.constFind(it.value()) != _SetToPretension.cend())
            {
                int connectID = _SetToPretension[it.value()];
                std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase = _modeldata->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(connectID);
                shared_ptr<MechConnectorData::mConnectorBolt> connect = dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(connectorBase);

                int setID = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(it.value(), true);
                MechSetData::mSetBase* setBase = _modeldata->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
                MechSetData::mNode* nodeSet = dynamic_cast<MechSetData::mNode*>(setBase);
                if (connect && nodeSet && nodeSet->getNodeData().size() > 0)
                {
                    connect->node_ = nodeSet->getNodeData()[0].result_id;
                    manager.appendMapRelation({ Physical_LoadBoundary, it.key() }, { Physical_Connector, _SetToPretension[it.value()] });
                }
            }
        }
    }
    void mIMxFile::readConnectingDeputy(const QString& propertyname, const QString& coordinatename)
    {
        QString name = ExistRenamePhysicalName(Physical_Deputy, propertyname);
        MechConnectorData::mDeputyManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);
        std::shared_ptr<MechConnectorData::mDeputyBase> baseData = manager.getDeputyData(name);
        std::shared_ptr<MechConnectorData::mConnectingDeputy> deputyData;
        if (baseData)
        {
            deputyData = dynamic_pointer_cast<MechConnectorData::mConnectingDeputy>(baseData);
        }
        else
        {
            deputyData = std::make_shared<MechConnectorData::mConnectingDeputy>(manager.createID(), Deputy_Connector, name);
            getCVM->getModelData()->addPhysicalFieldP(deputyData);
        }

        currentline = _file.readLine().simplified();
        if (deputyData == nullptr)
        {
            return;
        }

        appendMapPhysicalStruct(Physical_Deputy, deputyData->getDeputyId(), Physical_Coordinate, coordinatename);

        _linelist = currentline.split(",");
        QString Type1 = getQStringListIndexValue(_linelist, 0);
        if (!Type1.compare("join", Qt::CaseInsensitive))
        {
            deputyData->datatype_ = "球铰";
        }
        else if (!Type1.compare("slot", Qt::CaseInsensitive))
        {
            deputyData->datatype_ = "滑动";
        }
        else if (!Type1.compare("hinge", Qt::CaseInsensitive))
        {
            deputyData->datatype_ = "铰接";
        }
        else if (!Type1.compare("weld", Qt::CaseInsensitive))
        {
            deputyData->datatype_ = "焊接";
        }
        else if (!Type1.compare("cylindrical", Qt::CaseInsensitive))
        {
            deputyData->datatype_ = "圆柱";
        }
        else if (!Type1.compare("translator", Qt::CaseInsensitive))
        {
            deputyData->datatype_ = "平移";
        }

        QString Type2 = getQStringListIndexValue(_linelist, 1);
        if (!Type2.compare("revolute", Qt::CaseInsensitive))
        {
            deputyData->subdatatype_ = "旋转";
        }
        else if (!Type2.compare("align", Qt::CaseInsensitive))
        {
            deputyData->subdatatype_ = "对齐";
        }
        else if (!Type2.compare("universal", Qt::CaseInsensitive))
        {
            deputyData->subdatatype_ = "万向节";
        }

        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readBearingSection(const QString& propertyname)
    {
        MechConnectorData::mConnectorManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector);
        std::shared_ptr<MechConnectorData::mConnectorBase> baseData = manager.getConnectorData(propertyname);
        std::shared_ptr<MechConnectorData::mConnectorBearingUnits> bearingData;
        if (baseData)
        {
            bearingData = dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(baseData);
        }
        if (!bearingData)
        {
            currentline = _file.readLine().simplified();
            return;
        }
        QString rotate = getKeyValue(currentline, "rotatingplane", "=");
        if (rotate == "xy") bearingData->rotateFace_ = 0;
        else if (rotate == "xz") bearingData->rotateFace_ = 1;
        else bearingData->rotateFace_ = 2;
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        if (getQStringListIndexValue(_linelist, 0).toInt() == 0)
        {
            bearingData->defMode_ = 0;
            for (int i = 1; i < 9; ++i)
            {
                bearingData->bearingPara_.push_back(getQStringListIndexValue(_linelist, i).toDouble());
            }
        }
        else {
            while (!currentline.startsWith("*"))
            {
                bearingData->defMode_ = 1;
                QVector<double> temp;
                for (int i = 0; i < 9; ++i)
                {
                    temp.push_back(getQStringListIndexValue(_linelist, i).toDouble());
                }
                bearingData->curveRotate_.push_back(temp);
                if (_file.atEnd())
                    break;
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
            }
        }
    }
    void mIMxFile::readFastener()
    {
        MechConnectorData::mDeputyManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);
        int deputyID = manager.createID();
        std::shared_ptr<MechConnectorData::mConnectorFastener> deputyData = std::make_shared<MechConnectorData::mConnectorFastener>(deputyID, Deputy_Fastener);
        QString name = _modeldata->createNameByType(Physical_Deputy, deputyData->getDeputyTypeToStringEn());
        name = ExistRenamePhysicalName(Physical_Connector, name);
        deputyData->setDeputyName(name);

        getCVM->getModelData()->addPhysicalFieldP(deputyData);
        deputyData->defineType_ = 1;

        QString part = getKeyValue(currentline, "part", "=");
        appendMapPhysicalStruct(Physical_Deputy, deputyData->getDeputyId(), Physical_Deputy, part);

        currentline = _file.readLine().simplified();
        deputyData->influenceRadius_ = currentline.toDouble();

        vector<QString> partList;
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*", Qt::CaseInsensitive))
                break;
            if (currentline.isEmpty())
                continue;

            _linelist = currentline.split(",");
            for (int i = 0; i < _linelist.size(); i++)
            {
                part = getQStringListIndexValue(_linelist, i);
                if (part.isEmpty()) continue;
                partList.push_back(part);
            }
        }

        appendMapPhysicalTable(Physical_Deputy, deputyData->getDeputyId(), Physical_Part, partList);
    }
    void mIMxFile::readFreqSamplPointData()
    {

        // 1. 直接读取 type 和 name（原样读，原样创建）
        QString typeStr = getKeyValue(currentline, "type", "=");
        QString nameStr = getKeyValue(currentline, "name", "=");  //直接读名字

        // 2. 系统生成ID + 用读到的 name 直接创建面板
        int newId = _modeldata->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).createID();
        MechCaseData::mFreqSamplPointBase* panelData = new MechCaseData::mFreqSamplPointBase(newId, nameStr);  //直接用 name

        // 3. 加入模型
        _modeldata->addPhysicalField<MechCaseData::mFreqSamplPointBase>(panelData);

        // 4. 读取数据行
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",", QString::SkipEmptyParts);

        // 5. 根据类型勾选并填充数据
        if (typeStr == "freq")
        {
            panelData->_checked[0] = true;
            panelData->_freqSamplPoint.clear();
            for (const QString& val : _linelist)
            {
                panelData->_freqSamplPoint.append(val.toDouble());
            }
        }
        else if (typeStr == "freq1")
        {
            panelData->_checked[1] = true;
            if (_linelist.size() >= 3)
            {
                panelData->_freqSamplPoint1.startFreq = getQStringListIndexValue(_linelist, 0).toDouble();
                panelData->_freqSamplPoint1.stepFreq = getQStringListIndexValue(_linelist, 1).toDouble();
                panelData->_freqSamplPoint1.intervalNum = getQStringListIndexValue(_linelist, 2).toInt();
            }
        }
        else if (typeStr == "freq2")
        {
            panelData->_checked[2] = true;
            if (_linelist.size() >= 3)
            {
                panelData->_freqSamplPoint2.startFreq = getQStringListIndexValue(_linelist, 0).toDouble();
                panelData->_freqSamplPoint2.stepFreq = getQStringListIndexValue(_linelist, 1).toDouble();
                panelData->_freqSamplPoint2.totalPoints = getQStringListIndexValue(_linelist, 2).toInt();
            }
        }
        else if (typeStr == "freq5")
        {
            panelData->_checked[3] = true;
            if (_linelist.size() >= 5)
            {
                panelData->_freqSamplPoint3.startFreq = getQStringListIndexValue(_linelist, 0).toDouble();
                panelData->_freqSamplPoint3.endFreq = getQStringListIndexValue(_linelist, 1).toDouble();
                panelData->_freqSamplPoint3.refineNum = getQStringListIndexValue(_linelist, 2).toInt();
                panelData->_freqSamplPoint3.expandValue = getQStringListIndexValue(_linelist, 3).toDouble();
                panelData->_freqSamplPoint3.diffType = getQStringListIndexValue(_linelist, 4).toInt();
            }
        }
        else if (typeStr == "freq4")
        {
            panelData->_checked[4] = true;
            if (_linelist.size() >= 4)
            {
                panelData->_freqSamplPoint4.lowerFreq = getQStringListIndexValue(_linelist, 0).toDouble();
                panelData->_freqSamplPoint4.stepFreq = getQStringListIndexValue(_linelist, 1).toDouble();
                panelData->_freqSamplPoint4.shiftValue = getQStringListIndexValue(_linelist, 2).toDouble();
                panelData->_freqSamplPoint4.sampleNum = getQStringListIndexValue(_linelist, 3).toInt();
            }
        }
        // 6. 读取下一行，交给外部循环
        currentline = _file.readLine().simplified();
    }
    void mIMxFile::readLaminateData()
    {
        // ===================== 1. 全局单层 *layer, type=global =====================
        if (currentline.startsWith("*layer") && currentline.contains("type = global"))
        {
            QString nameStr = getKeyValue(currentline, "name", "=");  //直接读名字
            nameStr = ExistRenamePhysicalName(Physical_Laminate, nameStr);
            int newId = _modeldata->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).createID();
            MechPropertyData::mGlobalSglLam* GloSglam = new MechPropertyData::mGlobalSglLam(newId, GlobalSglLam, nameStr);
            _modeldata->addPhysicalField<MechPropertyData::mLaminateBase >(GloSglam);

            currentline = _file.readLine().simplified();

            QStringList linelist = currentline.split(",");
            appendMapPhysicalStruct(Physical_Laminate, GloSglam->getLaminateId(), Physical_Material, getQStringListIndexValue(linelist, 0));

            GloSglam->_theta = getQStringListIndexValue(linelist, 1);
            GloSglam->_thick = getQStringListIndexValue(linelist, 2);

            currentline = _file.readLine().simplified();
        }
        // ===================== 2. 普通单层 *layer =====================
        else if (currentline.startsWith("*layer"))
        {
            QString nameStr = getKeyValue(currentline, "name", "=");  //直接读名字
            nameStr = ExistRenamePhysicalName(Physical_Laminate, nameStr);
            int newId = _modeldata->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).createID();
            MechPropertyData::mSglLayerPlate* Sglam = new MechPropertyData::mSglLayerPlate(newId, SglLayerPlate, nameStr);
            _modeldata->addPhysicalField<MechPropertyData::mLaminateBase >(Sglam);

            QString setNameStr = getKeyValue(currentline, "shape", "=");  //集合名
            appendMapPhysicalStruct(Physical_Laminate, Sglam->getLaminateId(), Physical_Set, setNameStr);

            // 离散转角分布场
            QString distributionName = getKeyValue(currentline, "distribution", "=");
            appendMapPhysicalStruct(Physical_Laminate, Sglam->getLaminateId(), Physical_Field, distributionName);

            currentline = _file.readLine().simplified();

            QStringList linelist = currentline.split(",");
            appendMapPhysicalStruct(Physical_Laminate, Sglam->getLaminateId(), Physical_Material, getQStringListIndexValue(linelist, 0));

            Sglam->_theta = getQStringListIndexValue(linelist, 1);
            Sglam->_thick = getQStringListIndexValue(linelist, 2);

            currentline = _file.readLine().simplified();
        }
        // ===================== 1. 层合板 *stack =====================
        else if (currentline.startsWith("*stack"))
        {
            QString nameStr = getKeyValue(currentline, "name", "=");  //直接读名字
            nameStr = ExistRenamePhysicalName(Physical_Laminate, nameStr);
            int newId = _modeldata->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).createID();
            MechPropertyData::mLamPlate* lam = new MechPropertyData::mLamPlate(newId, LamPlate, nameStr);
            _modeldata->addPhysicalField<MechPropertyData::mLaminateBase >(lam);

            vector<QString> subNames;

            // 先读下一行（你现在的写法）
            currentline = _file.readLine().simplified();

            // 循环读取，直到遇到 * 停止
            while (!currentline.isEmpty())
            {
                // 遇到 * 号，结束读取
                if (currentline.startsWith("*"))
                    break;

                // 分割逗号
                QStringList parts = currentline.split(',', QString::SkipEmptyParts);

                // 逐个解析名称
                for (QString part : parts)
                {
                    QString name = part.trimmed();

                    // 去掉双引号
                    if (name.startsWith("\"") && name.endsWith("\""))
                    {
                        name = name.mid(1, name.length() - 2);
                    }

                    if (!name.isEmpty())
                    {
                        subNames.push_back(name);
                    }
                }
                // 继续读下一行
                currentline = _file.readLine().simplified();
            }

            if (!subNames.empty())
            {
                //用容器中的第一个判断下是单层板还是全局单层板
                MechPropertyData::mLaminateBase* data = _modeldata->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getLaminateData(subNames[0]);
                if (!data) return;
                if (data->getLaminateType() == SglLayerPlate)
                {
                    lam->_type = 0;
                }
                else if (data->getLaminateType() == GlobalSglLam)
                {
                    lam->_type = 1;
                }
                appendMapPhysicalTable(Physical_Laminate, lam->getLaminateId(), Physical_Laminate, subNames);
            }
        }
    }
    void mIMxFile::readLinearstatic(const QString& casename, int caseID)
    {
        QString inrel = getKeyValue(currentline, "inrel", "=");
        QString solver = getKeyValue(currentline, "solver", "=");
        QString autospc = getKeyValue(currentline, "autospc", "=");

        MechCaseData::mCaseImplicitLinearStatic* caseData = new MechCaseData::mCaseImplicitLinearStatic(caseID, ImplicitLinearStatic, casename);
        QHash<QString, QStringList> clearDefault;
        caseData->setOutputs(clearDefault);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);
        if (!inrel.isEmpty())
        {
            if (!inrel.compare("0", Qt::CaseInsensitive))
            {
                caseData->_inertiaRelief = "关闭";
            }
            else if (!inrel.compare("-1", Qt::CaseInsensitive))
            {
                caseData->_inertiaRelief = "手动开启";
            }
            else if (!inrel.compare("-2", Qt::CaseInsensitive))
            {
                caseData->_inertiaRelief = "自动开启";
            }
        }

        if (!autospc.isEmpty())
        {
            if (!autospc.compare("on", Qt::CaseInsensitive))
            {
                caseData->_autospc = "开启";
            }
            else if (!autospc.compare("off", Qt::CaseInsensitive))
            {
                caseData->_autospc = "关闭";
            }
        }

        if (!solver.isEmpty())
        {
            if (!solver.compare("dsv", Qt::CaseInsensitive))
            {
                caseData->_equationSolver = "直接法求解器DSV";
            }
            else if (!solver.compare("pariters", Qt::CaseInsensitive))
            {
                caseData->_equationSolver = "迭代法求解器Pariters";
            }
        }

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*load", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive) || currentline.startsWith("*suport1", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }
    }
    void mIMxFile::readNonlinear(const QString& casename, int caseID)
    {
        QString nlgeo = getKeyValue(currentline, "nlgeo", "=");
        QString direct = getKeyValue(currentline, "direct", "=");
        QString cntnlsub = getKeyValue(currentline, "cntnlsub", "=");
        QString inrel = getKeyValue(currentline, "inrel", "=");
        QString sparsematrix = getKeyValue(currentline, "sparsematrix", "=");

        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*static", Qt::CaseInsensitive) || currentline.startsWith("*visco", Qt::CaseInsensitive))
        {
            QString cetol = getKeyValue(currentline, "cetol", "=");
            MechCaseData::mCaseImplicitNonLinearStatic* caseData = new MechCaseData::mCaseImplicitNonLinearStatic(caseID, ImplicitNonLinearStatic, casename);
            getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);
            if (!nlgeo.isEmpty())
            {
                if (!nlgeo.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_nonLinearity = "开";
                }
                else if (!nlgeo.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_nonLinearity = "关";
                }
            }

            if (!cntnlsub.isEmpty())
            {
                if (!cntnlsub.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "开启";
                }
                else if (!cntnlsub.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "关闭";
                }
            }

            if (!inrel.isEmpty())
            {
                if (!inrel.compare("0", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "关闭";
                }
                else if (!inrel.compare("-1", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "手动开启";
                }
                else if (!inrel.compare("-2", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "自动开启";
                }
            }

            if (!direct.isEmpty())
            {
                if (!direct.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "固定";
                }
                else if (!direct.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "自动";
                }
            }

            if (!sparsematrix.isEmpty())
            {
                if (!sparsematrix.compare("symm", Qt::CaseInsensitive))
                {
                    caseData->_Sparsematrix = "关闭";
                }
                else if (!sparsematrix.compare("unsymm", Qt::CaseInsensitive))
                {
                    caseData->_Sparsematrix = "开启";
                }
            }

            if (!cetol.isEmpty())
            {
                caseData->_visco = true;
                caseData->_cetol = cetol;
            }

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            QString value;
            if (caseData->_timeStepType == "自动")
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_minTimeStep = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_maxTimeStep = value;
            }
            else
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
            }
        }
        else if (currentline.startsWith("*dynamic", Qt::CaseInsensitive))
        {
            MechCaseData::mCaseImplicitNonLinearDynamic* caseData = new MechCaseData::mCaseImplicitNonLinearDynamic(caseID, ImplicitNonLinearDynamic, casename);
            getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);
            if (!nlgeo.isEmpty())
            {
                if (!nlgeo.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_nonLinearity = "开";
                }
                else if (!nlgeo.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_nonLinearity = "关";
                }
            }

            if (!cntnlsub.isEmpty())
            {
                if (!cntnlsub.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "开启";
                }
                else if (!cntnlsub.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "关闭";
                }
            }

            if (!inrel.isEmpty())
            {
                if (!inrel.compare("0", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "关闭";
                }
                else if (!inrel.compare("-1", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "手动开启";
                }
                else if (!inrel.compare("-2", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "自动开启";
                }
            }

            if (!direct.isEmpty())
            {
                if (!direct.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "固定";
                }
                else if (!direct.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "自动";
                }
            }

            QString quasistatic = getKeyValue(currentline, "quasistatic", "=");
            if (!quasistatic.isEmpty())
            {
                if (!quasistatic.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_quasiStaticAnasys = "开";
                }
                else if (!quasistatic.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_quasiStaticAnasys = "关";
                }
            }

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            QString value;
            if (caseData->_timeStepType == "自动")
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_minTimeStep = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_maxTimeStep = value;
            }
            else
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;

            }
        }
        else if (currentline.startsWith("*arc", Qt::CaseInsensitive))
        {
            MechCaseData::mCaseImplicitNonLinearArc* caseData = new MechCaseData::mCaseImplicitNonLinearArc(caseID, ImplicitPostBuckling, casename);
            QHash<QString, QStringList> clearDefault;
            caseData->setOutputs(clearDefault);
            getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);

            if (!nlgeo.isEmpty())
            {
                if (!nlgeo.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_nonLinearity = "开";
                }
                else if (!nlgeo.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_nonLinearity = "关";
                }
            }

            if (!cntnlsub.isEmpty())
            {
                if (!cntnlsub.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "开启";
                }
                else if (!cntnlsub.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "关闭";
                }
            }

            if (!inrel.isEmpty())
            {
                if (!inrel.compare("0", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "关闭";
                }
                else if (!inrel.compare("-1", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "手动开启";
                }
                else if (!inrel.compare("-2", Qt::CaseInsensitive))
                {
                    caseData->_inertiaRelief = "自动开启";
                }
            }

            if (!direct.isEmpty())
            {
                if (!direct.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "固定";
                }
                else if (!direct.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "自动";
                }
            }

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            QString value;
            value = getQStringListIndexValue(_linelist, 0);
            caseData->_initialStepValue = value;
            value = getQStringListIndexValue(_linelist, 1);
            caseData->_totolTimeValue = value;
            value = getQStringListIndexValue(_linelist, 2);
            caseData->_minTimeStep = value;
            value = getQStringListIndexValue(_linelist, 3);
            caseData->_maxTimeStep = value;
            value = getQStringListIndexValue(_linelist, 4);
            caseData->_lMax = value;
            value = getQStringListIndexValue(_linelist, 8);
            caseData->_nMax = value;
        }
        else
        {
            currentline = _file.readLine().simplified();
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("分析类型 %1 暂不支持！").arg(casename));
            return;
        }

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*load", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*imperfection", Qt::CaseInsensitive))
            {
                readCaseInitBug(caseID);
            }
            else if (currentline.startsWith("*contactinterference", Qt::CaseInsensitive))
            {
                readCaseContactinterference(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }
    }
    void mIMxFile::readHeattransfer(const QString& casename, int caseID)
    {
        QString direct = getKeyValue(currentline, "direct", "=");
        QString cntnlsub = getKeyValue(currentline, "cntnlsub", "=");

        currentline = _file.readLine().simplified();
        if (currentline.startsWith("*steady", Qt::CaseInsensitive))
        {
            MechCaseData::mCaseImplicitSteadyHeat* caseData = new MechCaseData::mCaseImplicitSteadyHeat(caseID, ImplicitSteadyHeat, casename);
            QHash<QString, QStringList> clearDefault;
            caseData->setOutputs(clearDefault);
            getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);

            if (!cntnlsub.isEmpty())
            {
                if (!cntnlsub.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "开启";
                }
                else if (!cntnlsub.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "关闭";
                }
            }

            if (!direct.isEmpty())
            {
                if (!direct.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "固定";
                }
                else if (!direct.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "自动";
                }
            }

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            QString value;
            if (caseData->_timeStepType == "自动")
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_minTimeStep = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_maxTimeStep = value;
            }
            else
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
            }

        }
        else if (currentline.startsWith("*transient", Qt::CaseInsensitive))
        {
            MechCaseData::mCaseImplicitTransientHeat* caseData = new MechCaseData::mCaseImplicitTransientHeat(caseID, ImplicitTransientHeat, casename);
            QHash<QString, QStringList> clearDefault;
            caseData->setOutputs(clearDefault);
            getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);

            if (!cntnlsub.isEmpty())
            {
                if (!cntnlsub.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "开启";
                }
                else if (!cntnlsub.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "关闭";
                }
            }

            if (!direct.isEmpty())
            {
                if (!direct.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "固定";
                }
                else if (!direct.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "自动";
                }
            }

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            QString value;
            if (caseData->_timeStepType == "自动")
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_minTimeStep = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_maxTimeStep = value;
            }
            else
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
            }
        }
        else if (currentline.startsWith("*coupledtempdisp", Qt::CaseInsensitive))
        {
            MechCaseData::mCaseImplicitStructuralCouplingHeat* caseData = new MechCaseData::mCaseImplicitStructuralCouplingHeat(caseID, ImplicitStructuralCouplingHeat, casename);
            QHash<QString, QStringList> clearDefault;
            caseData->setOutputs(clearDefault);
            getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);

            if (!cntnlsub.isEmpty())
            {
                if (!cntnlsub.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "开启";
                }
                else if (!cntnlsub.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_cntnlsub = "关闭";
                }
            }

            if (!direct.isEmpty())
            {
                if (!direct.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "固定";
                }
                else if (!direct.compare("off", Qt::CaseInsensitive))
                {
                    caseData->_timeStepType = "自动";
                }
            }

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            QString value;
            if (caseData->_timeStepType == "自动")
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_minTimeStep = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_maxTimeStep = value;
            }
            else
            {
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_initialStepValue = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_totolTimeValue = value;
            }
        }
        else
        {
            currentline = _file.readLine().simplified();
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("分析类型 %1 暂不支持！").arg(currentline));
        }

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*load", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }

    }
    void mIMxFile::readModesComplexmodes(const QString& casename, int caseID)
    {
        QString type = getKeyValue(currentline, "type", "=");
        QString solver = getKeyValue(currentline, "solver", "=");
        QString method = getKeyValue(currentline, "method", "=");
        QString nlgeo = getKeyValue(currentline, "nlgeo", "=");
        MechCaseData::mCaseImplicitModal* caseData = new MechCaseData::mCaseImplicitModal(caseID, ImplicitModal, casename);
        QHash<QString, QStringList> clearDefault;
        caseData->setOutputs(clearDefault);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);

        if (!type.compare("modes", Qt::CaseInsensitive))
        {
            caseData->_modalType = "模态分析";
        }
        else if (!type.compare("complexmodes1", Qt::CaseInsensitive))
        {
            caseData->_modalType = "复模态分析";
        }
        else if (!type.compare("rotor_modes", Qt::CaseInsensitive))
        {
            caseData->_modalType = "复模态分析";
            caseData->_rotationalChecked = true;
        }

        if (!solver.isEmpty())
        {
            if (!solver.compare("amls", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "自动多重子结构";
            }
            else if (!solver.compare("blocklanczos", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "兰索斯";
            }
        }

        if (!method.isEmpty())
        {
            if (!method.compare("exact", Qt::CaseInsensitive))
            {
                caseData->_complexMethod = "精确法";
            }
            else if (!method.compare("approximate", Qt::CaseInsensitive))
            {
                caseData->_complexMethod = "估计法";
            }
        }

        if (!nlgeo.isEmpty() && !nlgeo.compare("on", Qt::CaseInsensitive))
        {
            caseData->_nlgeo = true;
        }

        currentline = _file.readLine().simplified();
        QString value;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*load", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*eigra", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                value = getQStringListIndexValue(_linelist, 4);
                caseData->_ampffact = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*eigrl", Qt::CaseInsensitive))
            {
                QString normalization = getKeyValue(currentline, "normalization", "=");
                if (!normalization.isEmpty())
                {
                    if (!normalization.compare("mass", Qt::CaseInsensitive))
                    {
                        caseData->_normalization = "广义质量";
                    }
                    else if (!normalization.compare("displacement", Qt::CaseInsensitive))
                    {
                        caseData->_normalization = "最大位移";
                    }
                }

                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*eigc", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                caseData->_rootNum = getQStringListIndexValue(_linelist, 0);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*damp", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_dampingG = value;
            }
            else if (currentline.startsWith("*preload", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Case, value);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*omega", Qt::CaseInsensitive))
            {
                readOmega(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            else if (currentline.startsWith("*coriolis", Qt::CaseInsensitive))
            {
                QString coor = getKeyValue(currentline, "system", "=");
                if (coor == "stationary")
                {
                    caseData->_standCoor = "静止坐标系";
                }
                else if (coor == "rotating")
                {
                    caseData->_standCoor = "旋转坐标系";
                }
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*plcamp", Qt::CaseInsensitive))
            {
                caseData->_picChecked = true;
                currentline = _file.readLine().simplified();
                caseData->_picValue = currentline;
                currentline = _file.readLine().simplified();
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }


    }
    void mIMxFile::readBuckling(const QString& casename, int caseID)
    {
        QString statsub = getKeyValue(currentline, "statsub", "=");
        MechCaseData::mCaseImplicitLinearBuckling* caseData = new MechCaseData::mCaseImplicitLinearBuckling(caseID, ImplicitLinearBuckling, casename);
        QHash<QString, QStringList> clearDefault;
        caseData->setOutputs(clearDefault);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);
        appendMapPhysicalStruct(Physical_Case, caseID, Physical_Case, statsub);

        currentline = _file.readLine().simplified();
        QString value;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*load", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*eigrl", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }
    }
    void mIMxFile::readResponseSpectrum(const QString& casename, int caseID)
    {
        QString type = getKeyValue(currentline, "type", "=");
        MechCaseData::mCaseImplicitResponseSpectrum* caseData = new MechCaseData::mCaseImplicitResponseSpectrum(caseID, ImplicitResponseSpectrum, casename);
        QHash<QString, QStringList> clearDefault;
        caseData->setOutputs(clearDefault);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);

        currentline = _file.readLine().simplified();
        QString value;
        int i = 0;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*dload", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*eigra", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "自动多重子结构";
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                value = getQStringListIndexValue(_linelist, 4);
                caseData->_ampffact = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*eigrl", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "兰索斯";
                QString normalization = getKeyValue(currentline, "normalization", "=");
                if (!normalization.isEmpty())
                {
                    if (!normalization.compare("mass", Qt::CaseInsensitive))
                    {
                        caseData->_normalization = "广义质量";
                    }
                    else if (!normalization.compare("displacement", Qt::CaseInsensitive))
                    {
                        caseData->_normalization = "最大位移";
                    }
                }

                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*damp", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_dampingG = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_dampingAlpha1 = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_dampingAlpha2 = value;
                value = getQStringListIndexValue(_linelist, 3);
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Function, value);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*rspec", Qt::CaseInsensitive))
            {
                QString dcomb = getKeyValue(currentline, "dcomb", "=");
                if (!dcomb.isEmpty())
                {
                    if (!dcomb.compare("ALG", Qt::CaseInsensitive))
                    {
                        caseData->_dirction = "ALG";
                    }
                    else if (!dcomb.compare("SRSS", Qt::CaseInsensitive))
                    {
                        caseData->_dirction = "SRSS";
                    }
                }
                QString mcomb = getKeyValue(currentline, "mcomb", "=");
                if (!mcomb.isEmpty())
                {
                    if (!mcomb.compare("ABS", Qt::CaseInsensitive))
                    {
                        caseData->_modal = "ABS";
                    }
                    else if (!mcomb.compare("SRSS", Qt::CaseInsensitive))
                    {
                        caseData->_modal = "SRSS";
                    }
                    else if (!mcomb.compare("CQC", Qt::CaseInsensitive))
                    {
                        caseData->_modal = "CQC";
                    }
                    else if (!mcomb.compare("NRL", Qt::CaseInsensitive))
                    {
                        caseData->_modal = "NRL";
                    }
                    else if (!mcomb.compare("DSUM", Qt::CaseInsensitive))
                    {
                        caseData->_modal = "DSUM";
                    }
                    else if (!mcomb.compare("ROSE", Qt::CaseInsensitive))
                    {
                        caseData->_modal = "ROSE";
                    }
                }
                QString close = getKeyValue(currentline, "close", "=");
                caseData->_modalFrequencyClose = close;
                vector<QString> names;
                while (!_file.atEnd())
                {
                    currentline = _file.readLine().simplified();
                    if (currentline.startsWith("*")) break;
                    _linelist = currentline.split(",");
                    MechCaseData::ResponseSpectrum_Data data;
                    value = getQStringListIndexValue(_linelist, 0);
                    names.push_back(value);
                    value = getQStringListIndexValue(_linelist, 1);
                    data._scales = value;
                    value = getQStringListIndexValue(_linelist, 2);
                    data._loadx = value;
                    value = getQStringListIndexValue(_linelist, 3);
                    data._loady = value;
                    value = getQStringListIndexValue(_linelist, 4);
                    data._loadz = value;
                    caseData->_tableData.append(data);
                }
                appendMapPhysicalTable(Physical_Case, caseID, Physical_LoadBoundary, names);
            }
            else if (currentline.startsWith("*preload", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Case, value);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*freq", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }

    }

    void mIMxFile::readRandomVibration(const QString& casename, int caseID)
    {
        QString type = getKeyValue(currentline, "type", "=");
        MechCaseData::mCaseImplicitRandomVibration* caseData = new MechCaseData::mCaseImplicitRandomVibration(caseID, ImplicitRandomVibration, casename);
        QHash<QString, QStringList> clearDefault;
        caseData->setOutputs(clearDefault);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);
        currentline = _file.readLine().simplified();
        QString value;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*randt", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_time0 = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_timeMax = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_nTime = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*loadsteps", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*caseoutput", Qt::CaseInsensitive))
            {
                while (!_file.atEnd())
                {
                    currentline = _file.readLine().simplified();
                    if (currentline.startsWith("*"))
                        break;
                    _linelist = currentline.split(",");

                    MechCaseData::RandomCase_OutPutType outPutObj;
                    outPutObj._outputType = getQStringListIndexValue(_linelist, 0);
                    outPutObj._randomType = getQStringListIndexValue(_linelist, 1);

                    QString nodeId = MxFunctions::mNumberToString(getQStringListIndexValue(_linelist, 2).toInt() + _lastMaxNodeID);
                    outPutObj._nodeNum = nodeId;

                    outPutObj._dofNum = getQStringListIndexValue(_linelist, 3);

                    caseData->m_outputData.append(outPutObj);
                }
            }
            else if (currentline.startsWith("*randps", Qt::CaseInsensitive))
            {
                while (!_file.atEnd())
                {
                    currentline = _file.readLine().simplified();
                    if (currentline.startsWith("*")) break;
                    _linelist = currentline.split(",");

                    RandomCase_OutPut outPut;
                    MechCaseData::RandomCase_RandPs randPs;
                    value = getQStringListIndexValue(_linelist, 0);
                    outPut.caseName1 = value;
                    value = getQStringListIndexValue(_linelist, 1);
                    outPut.caseName2 = value;
                    value = getQStringListIndexValue(_linelist, 2);
                    randPs._x = value;
                    value = getQStringListIndexValue(_linelist, 3);
                    randPs._y = value;
                    value = getQStringListIndexValue(_linelist, 4);
                    outPut.funcName = value;

                    _randomCaseOutPut[caseData->getCaseId()].append(outPut);
                    caseData->_tableXYData.append(randPs);
                }
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else if (currentline.startsWith("*"))
            {
                break;
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }
    }

    void mIMxFile::analysisRandomCaseOutPut()
    {
        int id;
        for (auto it = _randomCaseOutPut.begin(); it != _randomCaseOutPut.cend(); ++it)
        {
            int caseID = it.key();
            MechCaseData::mCaseBase* caseBase = _modeldata->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
            MechCaseData::mCaseImplicitRandomVibration* caseData = dynamic_cast<MechCaseData::mCaseImplicitRandomVibration*>(caseBase);
            if (caseData == nullptr) continue;

            int row = 0;
            for (auto& random : it.value())
            {
                id = _modeldata->getPhyIDByName(Physical_Case, random.caseName1);
                _modeldata->getMapRelationManager().appendTableMapRelation({ Physical_Case,caseID }, { Physical_Case, id }, row, MapRelation_TableCase1);
                id = _modeldata->getPhyIDByName(Physical_Case, random.caseName2);
                _modeldata->getMapRelationManager().updateTabelMapRelation({ Physical_Case,caseID }, { Physical_Case, id }, row, MapRelation_TableCase2);
                id = _modeldata->getPhyIDByName(Physical_Function, random.funcName);
                _modeldata->getMapRelationManager().updateTabelMapRelation({ Physical_Case,caseID }, { Physical_Case, id }, row, MapRelation_TableFunction);
                row++;
            }
        }
    }

    void mIMxFile::analysisPanleElementMap()
    {
        for (auto it = _panelToElements.begin(); it != _panelToElements.cend(); ++it)
        {
            int panelId = it.key();
            int row = 0;
            for (auto& eleName : it.value())
            {
                int elementId = _modeldata->getPhyIDByName(Physical_Set, eleName);
                _modeldata->getMapRelationManager().appendTableMapRelation({ Physical_Set, panelId }, { Physical_Set, elementId }, row);
                row++;
            }
        }
    }

    void mIMxFile::nonPropertyMap()
    {
        for (auto it = _nonToPropertyName.begin(); it != _nonToPropertyName.cend(); ++it)
        {
            int nonId = it.key();
            int row = 0;
            for (auto& properName : it.value())
            {
                int propertyId = _modeldata->getPhyIDByName(Physical_Property, properName);
                _modeldata->getMapRelationManager().appendTableMapRelation({ Physical_NonStructuralQuality, nonId },
                    { Physical_Property, propertyId }, row);
                row++;
            }

        }
    }
    void mIMxFile::nonElementMap()
    {
        for (auto it = _nonToElementId.begin(); it != _nonToElementId.cend(); ++it)
        {
            int nonId = it.key();
            int row = 0;
            for (auto& elementId : it.value())
            {
                _modeldata->getMapRelationManager().appendTableMapRelation({ Physical_NonStructuralQuality, nonId },
                    { Physical_Element, elementId }, row);
                row++;
            }
        }
    }
    void mIMxFile::nonElementSetMap()
    {
        for (auto it = _nonToSetName.begin(); it != _nonToSetName.cend(); ++it)
        {
            int nonId = it.key();
            int row = 0;
            for (auto& setName : it.value())
            {
                int eleSetId = _modeldata->getPhyIDByName(Physical_Set, setName);
                _modeldata->getMapRelationManager().appendTableMapRelation({ Physical_NonStructuralQuality, nonId },
                    { Physical_Set, eleSetId }, row);
                row++;
            }
        }
    }

    void mIMxFile::caseNsmMap()
    {
        for (auto it = _caseNsm.begin(); it != _caseNsm.cend(); ++it)
        {
            int caseId = it.key();
            int row = 0;
            for (auto& nsmName : it.value())
            {
                ////更新到映射
                int nsmId = _modeldata->getPhyIDByName(Physical_NonStructuralQuality, nsmName);
                _modeldata->getMapRelationManager().appendTableMapRelation({ Physical_Case, caseId },
                    { Physical_NonStructuralQuality, nsmId }, row);
                row++;
            }
        }
    }

    void mIMxFile::readASCoupling(const QString& casename, int caseID)
    {
        QString resvec = getKeyValue(currentline, "resvec", "=");
        QString type = getKeyValue(currentline, "type", "=");
        MechCaseData::mCaseImplASCoupling* caseData = new MechCaseData::mCaseImplASCoupling(caseID, ASCoupling, casename);
        QHash<QString, QStringList> clearDefault;
        caseData->setOutputs(clearDefault);
        _modeldata->addPhysicalField<MechCaseData::mCaseBase>(caseData);
        if (!type.compare("frequencyresponse_modal1", Qt::CaseInsensitive))
        {

            if (!resvec.isEmpty())
            {
                if (!resvec.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_isRemnantOpen = true;
                }
            }
        }


        currentline = _file.readLine().simplified();
        QString value;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*dload", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*eigra", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "自动多重子结构";

                QString eigraType = getKeyValue(currentline, "field", "=");
                if (!eigraType.compare("structure", Qt::CaseInsensitive))
                {
                    currentline = _file.readLine().simplified();
                    _linelist = currentline.split(",");
                    value = getQStringListIndexValue(_linelist, 0);
                    caseData->_structMinFrequency = value;
                    value = getQStringListIndexValue(_linelist, 1);
                    caseData->_structMaxFrequency = value;
                    value = getQStringListIndexValue(_linelist, 2);
                    caseData->_structOrder = value;
                    value = getQStringListIndexValue(_linelist, 3);
                    caseData->_structTolerance = value;
                    value = getQStringListIndexValue(_linelist, 4);
                    caseData->_structAmpffact = value;
                    currentline = _file.readLine().simplified();
                }
                else if (!eigraType.compare("fluid", Qt::CaseInsensitive))
                {
                    currentline = _file.readLine().simplified();
                    _linelist = currentline.split(",");
                    value = getQStringListIndexValue(_linelist, 0);
                    caseData->_acousticsMinFrequency = value;
                    value = getQStringListIndexValue(_linelist, 1);
                    caseData->_acousticsMaxFrequency = value;
                    value = getQStringListIndexValue(_linelist, 2);
                    caseData->_acousticsOrder = value;
                    value = getQStringListIndexValue(_linelist, 3);
                    caseData->_acousticsTolerance = value;
                    value = getQStringListIndexValue(_linelist, 4);
                    caseData->_acousticsAmpffact = value;
                    currentline = _file.readLine().simplified();
                }


            }
            else if (currentline.startsWith("*eigrl", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "兰索斯";

                QString eigrlType = getKeyValue(currentline, "field", "=");
                if (!eigrlType.compare("structure", Qt::CaseInsensitive))
                {
                    QString structNormalization = getKeyValue(currentline, "normalization", "=");
                    if (!structNormalization.isEmpty())
                    {
                        if (!structNormalization.compare("mass", Qt::CaseInsensitive))
                        {
                            caseData->_structNormalization = "广义质量";
                        }
                        else if (!structNormalization.compare("displacement", Qt::CaseInsensitive))
                        {
                            caseData->_structNormalization = "最大位移";
                        }
                    }

                    currentline = _file.readLine().simplified();
                    _linelist = currentline.split(",");
                    value = getQStringListIndexValue(_linelist, 0);
                    caseData->_structMinFrequency = value;
                    value = getQStringListIndexValue(_linelist, 1);
                    caseData->_structMaxFrequency = value;
                    value = getQStringListIndexValue(_linelist, 2);
                    caseData->_structOrder = value;
                    value = getQStringListIndexValue(_linelist, 3);
                    caseData->_structTolerance = value;
                    currentline = _file.readLine().simplified();
                }
                else if (!eigrlType.compare("fluid", Qt::CaseInsensitive))
                {
                    QString acousticsNormalization = getKeyValue(currentline, "normalization", "=");
                    if (!acousticsNormalization.isEmpty())
                    {
                        if (!acousticsNormalization.compare("mass", Qt::CaseInsensitive))
                        {
                            caseData->_acousticsNormalization = "广义质量";
                        }
                        else if (!acousticsNormalization.compare("displacement", Qt::CaseInsensitive))
                        {
                            caseData->_acousticsNormalization = "最大位移";
                        }
                    }

                    currentline = _file.readLine().simplified();
                    _linelist = currentline.split(",");
                    value = getQStringListIndexValue(_linelist, 0);
                    caseData->_acousticsMinFrequency = value;
                    value = getQStringListIndexValue(_linelist, 1);
                    caseData->_acousticsMaxFrequency = value;
                    value = getQStringListIndexValue(_linelist, 2);
                    caseData->_acousticsOrder = value;
                    value = getQStringListIndexValue(_linelist, 3);
                    caseData->_acousticsTolerance = value;
                    currentline = _file.readLine().simplified();
                }


            }
            else if (currentline.startsWith("*damp", Qt::CaseInsensitive))
            {
                QString dampType = getKeyValue(currentline, "field", "=");
                if (!dampType.compare("structure", Qt::CaseInsensitive))
                {
                    currentline = _file.readLine().simplified();
                    _linelist = currentline.split(",");
                    value = getQStringListIndexValue(_linelist, 0);
                    caseData->_structDampingG = value;
                    value = getQStringListIndexValue(_linelist, 1);
                    caseData->_structDampingAlpha1 = value;
                    value = getQStringListIndexValue(_linelist, 2);
                    caseData->_structDampingAlpha2 = value;
                    value = getQStringListIndexValue(_linelist, 3);
                    appendMapPhysicalStruct(Physical_Case, caseID, Physical_Function, value);
                    currentline = _file.readLine().simplified();
                }
                else if (!dampType.compare("fluid", Qt::CaseInsensitive))
                {
                    currentline = _file.readLine().simplified();
                    _linelist = currentline.split(",");
                    value = getQStringListIndexValue(_linelist, 0);
                    caseData->_acousticsDampingG = value;
                    value = getQStringListIndexValue(_linelist, 1);
                    caseData->_acousticsDampingAlpha1 = value;
                    value = getQStringListIndexValue(_linelist, 2);
                    caseData->_acousticsDampingAlpha2 = value;
                    value = getQStringListIndexValue(_linelist, 3);
                    appendMapPhysicalStruct(Physical_Case, caseID, Physical_Function, value);
                    currentline = _file.readLine().simplified();
                }


            }
            else if (currentline.startsWith("*preload", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Case, value);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*history", Qt::CaseInsensitive))
            {
                readHistoryOutput(caseID);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*elforce", Qt::CaseInsensitive))
            {
                QString set = getKeyValue(currentline, "set", "=");
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Set, set, MapRelation_ElforceSet);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*gpforce", Qt::CaseInsensitive))
            {
                QString set = getKeyValue(currentline, "set", "=");
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Set, set, MapRelation_GpforceSet);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*mcf", Qt::CaseInsensitive))
            {
                mModalContriData* modalData = caseData->createModalContriData();
                readModalContriData(caseID, modalData);
            }
            else if (currentline.startsWith("*pfgrid", Qt::CaseInsensitive))
            {
                mNodeContriData* nodeData = caseData->createNodeContriData();
                readNodeContriData(caseID, nodeData);
            }
            else if (currentline.startsWith("*pfpanel", Qt::CaseInsensitive))
            {
                mPanelContriData* panelData = caseData->createPanelContriData();
                readPanelContriData(caseID, panelData);
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*freq", Qt::CaseInsensitive))
            {
                // 读取 *freq 下所有行，直到遇到 * 开头的关键字
                currentline = _file.readLine().simplified();

                // 存储所有 freq 名称
                vector<QString> freqNames;

                while (!_file.atEnd())
                {
                    // 遇到新的 * 关键字，停止读取
                    if (currentline.startsWith("*"))
                    {
                        break;
                    }

                    // 空行跳过
                    if (currentline.isEmpty())
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }

                    // 添加到列表（自动去掉引号）
                    QString freqName = currentline;
                    freqName.remove('"');  // 去掉 "
                    freqNames.push_back(freqName);

                    // 读取下一行
                    currentline = _file.readLine().simplified();
                }

                if (!freqNames.empty())
                {
                    caseData->checked = true;
                    appendMapPhysicalTable(Physical_Case, caseID, Physical_FreqSamplPoint, freqNames);
                }
            }
            else if (currentline.startsWith("*erp", Qt::CaseInsensitive))
            {
                readERPData();
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            else if (currentline.startsWith("*peakout", Qt::CaseInsensitive))
            {
                QString name = getKeyValue(currentline, "name", "=");
                if (!name.isEmpty())
                {
                    readPeakFilterData();
                }
                else
                {
                    currentline = _file.readLine().simplified();
                    std::vector<QString> pfNames;

                    while (!_file.atEnd())
                    {
                        if (currentline.startsWith("*"))
                        {
                            break;
                        }
                        if (currentline.isEmpty())
                        {
                            currentline = _file.readLine().simplified();
                            continue;
                        }

                        QString pfName = currentline;
                        pfName.remove('"');
                        pfNames.push_back(pfName);

                        currentline = _file.readLine().simplified();
                    }

                    if (!pfNames.empty())
                    {
                        appendMapPhysicalTable(Physical_Case, caseID, Physical_PeakFilter, pfNames);
                    }
                }
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }
    }
    void mIMxFile::readFrequencyresponse(const QString& casename, int caseID)
    {
        QString resvec = getKeyValue(currentline, "resvec", "=");
        QString type = getKeyValue(currentline, "type", "=");
        MechCaseData::mCaseImplicitFrequency* caseData = new MechCaseData::mCaseImplicitFrequency(caseID, ImplicitFrequency, casename);
        QHash<QString, QStringList> clearDefault;
        caseData->setOutputs(clearDefault);
        _modeldata->addPhysicalField<MechCaseData::mCaseBase>(caseData);
        if (!type.compare("frequencyresponse_modal", Qt::CaseInsensitive))
        {
            caseData->_method = "模态法";
            if (!resvec.isEmpty())
            {
                if (!resvec.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_isRemnantOpen = true;
                }
            }
        }
        else if (!type.compare("frequencyresponse_direct", Qt::CaseInsensitive))
        {
            caseData->_method = "直接法";
        }

        currentline = _file.readLine().simplified();
        QString value;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*dload", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*eigra", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "自动多重子结构";
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                value = getQStringListIndexValue(_linelist, 4);
                caseData->_ampffact = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*eigrl", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "兰索斯";
                QString normalization = getKeyValue(currentline, "normalization", "=");
                if (!normalization.isEmpty())
                {
                    if (!normalization.compare("mass", Qt::CaseInsensitive))
                    {
                        caseData->_normalization = "广义质量";
                    }
                    else if (!normalization.compare("displacement", Qt::CaseInsensitive))
                    {
                        caseData->_normalization = "最大位移";
                    }
                }

                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*damp", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_dampingG = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_dampingAlpha1 = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_dampingAlpha2 = value;
                value = getQStringListIndexValue(_linelist, 3);
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Function, value);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*preload", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Case, value);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*history", Qt::CaseInsensitive))
            {
                readHistoryOutput(caseID);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*elforce", Qt::CaseInsensitive))
            {
                QString set = getKeyValue(currentline, "set", "=");
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Set, set, MapRelation_ElforceSet);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*gpforce", Qt::CaseInsensitive))
            {
                QString set = getKeyValue(currentline, "set", "=");
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Set, set, MapRelation_GpforceSet);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*mcf", Qt::CaseInsensitive))
            {
                mModalContriData* modalData = caseData->createModalContriData();
                readModalContriData(caseID, modalData);
            }
            else if (currentline.startsWith("*peakout", Qt::CaseInsensitive))
            {
                QString name = getKeyValue(currentline, "name", "=");
                if (!name.isEmpty())
                {//读全局
                    readPeakFilterData();
                }
                else
                {
                    currentline = _file.readLine().simplified();
                    std::vector<QString> pfNames;

                    while (!_file.atEnd())
                    {
                        if (currentline.startsWith("*"))
                        {
                            break;
                        }
                        if (currentline.isEmpty())
                        {
                            currentline = _file.readLine().simplified();
                            continue;
                        }

                        QString pfName = currentline;
                        pfName.remove('"');
                        pfNames.push_back(pfName);

                        currentline = _file.readLine().simplified();
                    }

                    if (!pfNames.empty())
                    {
                        appendMapPhysicalTable(Physical_Case, caseID, Physical_PeakFilter, pfNames);
                    }
                }
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*freq", Qt::CaseInsensitive))
            {
                // 读取 *freq 下所有行，直到遇到 * 开头的关键字
                currentline = _file.readLine().simplified();

                // 存储所有 freq 名称
                vector<QString> freqNames;

                while (!_file.atEnd())
                {
                    // 遇到新的 * 关键字，停止读取
                    if (currentline.startsWith("*"))
                    {
                        break;
                    }

                    // 空行跳过
                    if (currentline.isEmpty())
                    {
                        currentline = _file.readLine().simplified();
                        continue;
                    }

                    // 添加到列表（自动去掉引号）
                    QString freqName = currentline;
                    freqName.remove('"');  // 去掉 "
                    freqNames.push_back(freqName);

                    // 读取下一行
                    currentline = _file.readLine().simplified();
                }

                if (!freqNames.empty())
                {
                    caseData->checked = true;
                    appendMapPhysicalTable(Physical_Case, caseID, Physical_FreqSamplPoint, freqNames);
                }
            }
            else if (currentline.startsWith("*erp", Qt::CaseInsensitive))
            {
                readERPData();
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }
    }
    void mIMxFile::readLineartransient(const QString& casename, int caseID)
    {
        QString resvec = getKeyValue(currentline, "resvec", "=");
        QString method = getKeyValue(currentline, "method", "=");
        QString type = getKeyValue(currentline, "type", "=");
        MechCaseData::mCaseImplicitTransient* caseData = new MechCaseData::mCaseImplicitTransient(caseID, ImplicitTransient, casename);
        QHash<QString, QStringList> clearDefault;
        caseData->setOutputs(clearDefault);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mCaseBase>(caseData);
        if (!type.compare("lineartransient_modal", Qt::CaseInsensitive))
        {
            caseData->_method = "模态法";
            if (!resvec.isEmpty())
            {
                if (!resvec.compare("on", Qt::CaseInsensitive))
                {
                    caseData->_isRemnantOpen = true;
                }
            }
        }
        else if (!type.compare("lineartransient_direct", Qt::CaseInsensitive))
        {
            caseData->_method = "直接法";

            if (!method.isEmpty())
            {
                if (!method.compare("newmark", Qt::CaseInsensitive))
                {
                    caseData->_solveMethod = "瞬态newmark法";
                }
                else if (!method.compare("hht", Qt::CaseInsensitive))
                {
                    caseData->_solveMethod = "瞬态hht法";
                }
                else if (!method.compare("wilson", Qt::CaseInsensitive))
                {
                    caseData->_solveMethod = "瞬态wilson法";
                }
            }

        }

        currentline = _file.readLine().simplified();
        QString value;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*dload", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (currentline.startsWith("*timestep", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_timestepCount = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_timestepVal = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*eigra", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "自动多重子结构";
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                value = getQStringListIndexValue(_linelist, 4);
                caseData->_ampffact = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*eigrl", Qt::CaseInsensitive))
            {
                caseData->_modalSolver = "兰索斯";
                QString normalization = getKeyValue(currentline, "normalization", "=");
                if (!normalization.isEmpty())
                {
                    if (!normalization.compare("mass", Qt::CaseInsensitive))
                    {
                        caseData->_normalization = "广义质量";
                    }
                    else if (!normalization.compare("displacement", Qt::CaseInsensitive))
                    {
                        caseData->_normalization = "最大位移";
                    }
                }

                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_minFrequency = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_maxFrequency = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_order = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_tolerance = value;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*damp", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                caseData->_dampingG = value;
                value = getQStringListIndexValue(_linelist, 1);
                caseData->_dampingOmg3 = value;
                value = getQStringListIndexValue(_linelist, 2);
                caseData->_dampingAlpha1 = value;
                value = getQStringListIndexValue(_linelist, 3);
                caseData->_dampingAlpha2 = value;
                value = getQStringListIndexValue(_linelist, 4);
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Function, value);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*preload", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                value = getQStringListIndexValue(_linelist, 0);
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_Case, value);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*output", Qt::CaseInsensitive))
            {
                readCaseOutPut(caseID);
            }
            else if (currentline.startsWith("*nsm", Qt::CaseInsensitive))
            {
                readCaseNsm(caseID);
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            else if (currentline.startsWith("*history", Qt::CaseInsensitive))
            {
                readHistoryOutput(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }


    }
    void mIMxFile::readGeneric(const QString& casename, int caseID)
    {
        MechCaseData::mCaseImplicitGeneric* caseData = new MechCaseData::mCaseImplicitGeneric(caseID, ImplicitGeneric, casename);
        _modeldata->addPhysicalField<MechCaseData::mCaseBase>(caseData);

        vector<int> partIDs = _modeldata->getAllPhyID(Physical_Part);
        if (partIDs.size() > 0)
        {
            mGenericPartData* genericPart = _modeldata->getGlobalParam()->getGenericPartData();
            if (genericPart)
            {
                _modeldata->deleteByPhysicalType(Physical_GenericPart, _modeldata->getModelDataID());
            }
            genericPart = _modeldata->getGlobalParam()->createGenericPartData();

            for (int i = 0; i < partIDs.size(); i++)
            {
                _modeldata->getMapRelationManager().appendTableMapRelation({ Physical_GenericPart,  getCVM->getModelData()->getModelDataID() },
                    { Physical_Part, partIDs[i] },
                    i);
            }
        }

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*dload", Qt::CaseInsensitive))
            {
                readCaseLoad(caseID);
            }
            else if (currentline.startsWith("*spc", Qt::CaseInsensitive))
            {
                readCaseBC(caseID);
            }
            else if (!currentline.compare("*cms", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                appendMapPhysicalStruct(Physical_Case, caseID, Physical_GenericParam, getQStringListIndexValue(_linelist, 0));
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*ic", Qt::CaseInsensitive))
            {
                readCaseInitTemp(caseID);
            }
            else if (currentline.startsWith("*temp_load", Qt::CaseInsensitive))
            {
                readCaseTempLoadField(caseID);
            }
            //读取case下模型修复
            else if (currentline.startsWith("*modelchange", Qt::CaseInsensitive))
            {
                readCaseModelFix(caseID);
            }
            else
            {
                if (currentline.startsWith("*"))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }

    }
    void mIMxFile::ReadStructOptionTopopara()
    {
        MechStructOptionData::mStructOption* opt = _modeldata->getStructOptionBase();

        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*"))
                break;

            _linelist = currentline.split(",");
            if (currentline.startsWith("rmin", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isRmin = true;
                opt->_mStructOptionData._rmin = _linelist[1].simplified();
            }
            else if (currentline.startsWith("maxiter", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isMaxiter = true;
                opt->_mStructOptionData._maxiter = _linelist[1].simplified();
            }
            else if (currentline.startsWith("discretetarget", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isDiscretetarget = true;
                opt->_mStructOptionData._discretetarget = _linelist[1].simplified();
            }
            else if (currentline.startsWith("penal", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isPenal = true;
                opt->_mStructOptionData._penal = _linelist[1].simplified();
            }
            else if (currentline.startsWith("topomovelimit", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isTopomovelimit = true;
                opt->_mStructOptionData._topomovelimit = _linelist[1].simplified();
            }
            else if (currentline.startsWith("mindim", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isRmin = true;
                opt->_mStructOptionData._rmin = _linelist[1].simplified();
            }
            else if (currentline.startsWith("objectivetolrance", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isObjectivetolrance = true;
                opt->_mStructOptionData._objectivetolrance = _linelist[1].simplified();
            }
            else if (currentline.startsWith("initdensity", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isInitdensity = true;
                opt->_mStructOptionData._initdensity = _linelist[1].simplified();
            }
            else if (currentline.startsWith("minDensity", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isMinDesity = true;
                opt->_mStructOptionData._minDesity = _linelist[1].simplified();
            }
            else if (currentline.startsWith("checker", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isChecker = true;
                opt->_mStructOptionData._checker = _linelist[1].simplified();
            }
            else if (currentline.startsWith("Rebuild", Qt::CaseInsensitive) && _linelist.size() >= 2)
            {
                opt->_mStructOptionData._isRebuild = true;
                opt->_mStructOptionData._rebuild = _linelist[1].simplified();
            }
        }
    }
    void mIMxFile::ReadStructOptionDesign()
    {

        auto QStringToVector3D = [this](QVector3D& data, int& node) {
            QString value;
            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            value = getQStringListIndexValue(_linelist, 0);
            if (value.toInt() == 0)
            {
                data = QVector3D(getQStringListIndexValue(_linelist, 1).toDouble(), getQStringListIndexValue(_linelist, 2).toDouble(), getQStringListIndexValue(_linelist, 3).toDouble());
            }
            else if (value.toInt() == 1)
            {
                node = getQStringListIndexValue(_linelist, 1).toInt() + _lastMaxNodeID;
            }
            };


        MechStructOptionData::mStructOption* opt = _modeldata->getStructOptionBase();

        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_OptimationDesign, name);

        QString type = getKeyValue(currentline, "type", "=");
        QString method = getKeyValue(currentline, "method", "=");
        QString secname = getKeyValue(currentline, "secname", "=");
        int id = opt->createOptimizationDesignID();
        MechStructOptionData::mOptimizationDesignData* design = new MechStructOptionData::mOptimizationDesignData(id, name);
        opt->addOptimizationDesignData(design);
        appendMapPhysicalStruct(Physical_OptimationDesign, id, Physical_Property, secname);
        if (!type.compare("shellsection", Qt::CaseInsensitive))
        {
            design->_designDomainType = "壳";
        }
        else if (!type.compare("solidsection", Qt::CaseInsensitive))
        {
            design->_designDomainType = "实体";
        }


        if (!method.compare("thick", Qt::CaseInsensitive))
        {
            design->_optimizationMethod = "厚度";
        }
        else if (!method.compare("rib", Qt::CaseInsensitive))
        {
            design->_optimizationMethod = "加筋";
        }
        else if (!method.compare("topography", Qt::CaseInsensitive))
        {
            design->_optimizationMethod = "形貌";
        }

        currentline = _file.readLine().simplified();
        QString value;
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*membersize", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");

                value = getQStringListIndexValue(_linelist, 0);
                if (!value.isEmpty())
                {
                    design->_minMemberStatus = true;
                    design->_minMemberSize = value;
                }

                value = getQStringListIndexValue(_linelist, 1);
                if (!value.isEmpty())
                {
                    design->_maxMemberStatus = true;
                    design->_maxMemberSize = value;
                }

                value = getQStringListIndexValue(_linelist, 2);
                if (!value.isEmpty())
                {
                    design->_minGapStatus = true;
                    design->_minGapSize = value;
                }
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*stressconstraint", Qt::CaseInsensitive))
            {
                design->_stressConstraintStatus = true;
                currentline = _file.readLine().simplified();
                if (!currentline.isEmpty())
                {
                    design->_stressConstraintValue = currentline;
                }
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*basicthick", Qt::CaseInsensitive))
            {
                currentline = _file.readLine().simplified();
                design->_baseThickness = currentline;
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*ribsize", Qt::CaseInsensitive))
            {
                if (design->_optimizationMethod == "形貌")
                {
                    currentline = _file.readLine().simplified();
                    _linelist = currentline.split(",");
                    value = getQStringListIndexValue(_linelist, 0);
                    design->_morphUpLimit = value;
                    value = getQStringListIndexValue(_linelist, 1);
                    design->_morphDownLimit = value;
                    currentline = _file.readLine().simplified();
                }
                else
                {
                    currentline = _file.readLine().simplified();
                    _linelist = currentline.split(",");
                    value = getQStringListIndexValue(_linelist, 0);
                    if (!value.isEmpty())
                    {
                        design->_ribHeightStatus = true;
                        design->_ribHeight = value;
                    }

                    value = getQStringListIndexValue(_linelist, 1);
                    if (!value.isEmpty())
                    {
                        design->_ribWidthStatus = true;
                        design->_ribWidth = value;
                    }
                    currentline = _file.readLine().simplified();
                }
            }
            else if (currentline.startsWith("*patterngrouping", Qt::CaseInsensitive))
            {
                design->_patternStatus = true;
                type = getKeyValue(currentline, "type", "=");
                if (!type.compare("planesym1", Qt::CaseInsensitive))
                {
                    design->_patternType = "单平面";
                    QStringToVector3D(design->_origin, design->_originId);
                    QStringToVector3D(design->_dirPoint1, design->_dirPoint1Id);
                }
                else if (!type.compare("planesym2"))
                {
                    design->_patternType = "双平面";
                    QStringToVector3D(design->_origin, design->_originId);
                    QStringToVector3D(design->_dirPoint1, design->_dirPoint1Id);
                    QStringToVector3D(design->_dirPoint2, design->_dirPoint2Id);
                }
                else if (!type.compare("planesym3"))
                {
                    design->_patternType = "三平面";
                    QStringToVector3D(design->_origin, design->_originId);
                    QStringToVector3D(design->_dirPoint1, design->_dirPoint1Id);
                    QStringToVector3D(design->_dirPoint2, design->_dirPoint2Id);
                }
                else if (!type.compare("cyclic"))
                {
                    design->_patternType = "周向";
                    currentline = _file.readLine().simplified();
                    design->_patternNum = currentline;
                    QStringToVector3D(design->_origin, design->_originId);
                    QStringToVector3D(design->_dirPoint1, design->_dirPoint1Id);
                }
                else if (!type.compare("cyclicplane"))
                {
                    design->_patternType = "周向+平面";
                    currentline = _file.readLine().simplified();
                    design->_patternNum = currentline;
                    QStringToVector3D(design->_origin, design->_originId);
                    QStringToVector3D(design->_dirPoint1, design->_dirPoint1Id);
                    QStringToVector3D(design->_dirPoint2, design->_dirPoint2Id);
                }
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*draw", Qt::CaseInsensitive))
            {
                design->_liftingStatus = true;
                type = getKeyValue(currentline, "type", "=");
                QString nohole = getKeyValue(currentline, "nohole", "=");
                if (!type.compare("single", Qt::CaseInsensitive))
                {
                    design->_liftingType = "单向";
                }
                else if (!type.compare("split", Qt::CaseInsensitive))
                {
                    design->_liftingType = "双向";
                }

                if (!nohole.compare("on", Qt::CaseInsensitive))
                {
                    design->_nohole = "开启";
                }

                QStringToVector3D(design->_startPoint, design->_startPointId);
                QStringToVector3D(design->_dirPoint, design->_dirPointId);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*extrusion", Qt::CaseInsensitive))
            {
                design->_extrusionStatus = true;
                type = getKeyValue(currentline, "type", "=");

                auto readPath = [this]() {
                    QVector<int> path;

                    while (!_file.atEnd())
                    {
                        currentline = _file.readLine().simplified();
                        if (currentline.isEmpty()) continue;
                        if (currentline.startsWith("*"))
                            break;

                        _linelist = currentline.split(",");
                        for (int i = 0; i < _linelist.size(); i++)
                        {
                            int node = _linelist[i].toInt() + _lastMaxNodeID;
                            path.push_back(node);
                        }
                    }

                    return path;
                    };

                currentline = _file.readLine().simplified(); //*path
                if (!type.compare("notwist", Qt::CaseInsensitive))
                {
                    design->_extrusionType = "平行";
                    design->_path1 = readPath();
                }
                else if (!type.compare("twist", Qt::CaseInsensitive))
                {
                    design->_extrusionType = "扭转";
                    design->_path1 = readPath();
                    design->_path2 = readPath();
                }
            }
            else if (currentline.startsWith("*overhang", Qt::CaseInsensitive))
            {
                type = getKeyValue(currentline, "type", "=");
                design->_supportStatus = true;
                currentline = _file.readLine().simplified();
                design->_overhangAngle = currentline;
                QStringToVector3D(design->_supportStartPoint, design->_supportStartPointId);
                QStringToVector3D(design->_supportDirPoint, design->_supportDirPointId);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*patternrepetition", Qt::CaseInsensitive))
            {
                design->_patternRepStatus = true;
                type = getKeyValue(currentline, "type", "=");
                QString target = getKeyValue(currentline, "target", "=");
                if (!type.compare("master", Qt::CaseInsensitive))
                {
                    design->_domainType = "主设计域";
                }
                else if (!type.compare("slave", Qt::CaseInsensitive))
                {
                    design->_domainType = "从设计域";
                    appendMapPhysicalStruct(Physical_OptimationDesign, id, Physical_OptimationDesign, target);
                }

                currentline = _file.readLine().simplified();  //*coord
                type = getKeyValue(currentline, "type", "=");

                if (!type.compare("globalsystem", Qt::CaseInsensitive))
                {
                    design->_coordType = "全局坐标系";
                    QStringToVector3D(design->_repBasePoint, design->_repBasePointId);
                }
                else if (!type.compare("point", Qt::CaseInsensitive))
                {
                    design->_coordType = "指定点";
                    QStringToVector3D(design->_repBasePoint, design->_repBasePointId);
                    QStringToVector3D(design->_repDirPoint1, design->_repDirPoint1Id);
                    QStringToVector3D(design->_repDirPoint2, design->_repDirPoint2Id);
                    QStringToVector3D(design->_repDirPoint3, design->_repDirPoint3Id);
                }

                currentline = _file.readLine().simplified();  //*scale
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");
                design->_scaleX = getQStringListIndexValue(_linelist, 0);
                design->_scaleY = getQStringListIndexValue(_linelist, 1);
                design->_scaleZ = getQStringListIndexValue(_linelist, 2);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*frozen", Qt::CaseInsensitive))
            {
                QString setname = getKeyValue(currentline, "set", "=");
                appendMapPhysicalStruct(Physical_OptimationDesign, id, Physical_Set, setname);
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*"))
            {
                break;
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }
    }
    void mIMxFile::ReadStructOptionConstraint()
    {
        MechStructOptionData::mStructOption* opt = _modeldata->getStructOptionBase();
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_OptimationConstraint, name);
        int id = opt->createOptimizationConstraintID();
        MechStructOptionData::mOptimizationConstraintData* constraint = new MechStructOptionData::mOptimizationConstraintData(id, name);
        opt->addOptimizationConstraintData(constraint);

        currentline = _file.readLine().simplified();
        while (!_file.atEnd())
        {
            if (currentline.startsWith("*bound", Qt::CaseInsensitive))
            {
                QString lower = getKeyValue(currentline, "lower", "=");
                QString upper = getKeyValue(currentline, "upper", "=");
                currentline = _file.readLine().simplified();
                _linelist = currentline.split(",");

                if (!lower.compare("on", Qt::CaseInsensitive))
                {
                    constraint->_lowerBoundStatus = true;
                    constraint->_lowerBoundValue = getQStringListIndexValue(_linelist, 0);
                }

                if (!upper.compare("on", Qt::CaseInsensitive))
                {
                    constraint->_upperBoundStatus = true;
                    constraint->_upperBoundValue = getQStringListIndexValue(_linelist, 1);
                }
                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*response", Qt::CaseInsensitive))
            {
                QString type = getKeyValue(currentline, "type", "=");
                QString targetsec = getKeyValue(currentline, "targetsec", "=");
                appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Property, targetsec);
                if (!type.compare("mass", Qt::CaseInsensitive) || !type.compare("totalmass", Qt::CaseInsensitive))
                    constraint->_constraintType = "质量";
                else if (!type.compare("massfrac", Qt::CaseInsensitive) || !type.compare("totalmassfrac", Qt::CaseInsensitive))
                    constraint->_constraintType = "质量分数";
                else if (!type.compare("volume", Qt::CaseInsensitive) || !type.compare("totalvolume", Qt::CaseInsensitive))
                    constraint->_constraintType = "体积";
                else if (!type.compare("volfrac", Qt::CaseInsensitive) || !type.compare("totalvolfrac", Qt::CaseInsensitive))
                    constraint->_constraintType = "体积分数";
                else if (!type.compare("compliance", Qt::CaseInsensitive) || !type.compare("stress", Qt::CaseInsensitive)
                    || !type.compare("thermalcompliance", Qt::CaseInsensitive) || !type.compare("frequency", Qt::CaseInsensitive)
                    || !type.compare("temperature", Qt::CaseInsensitive) || !type.compare("displacement", Qt::CaseInsensitive)
                    || !type.compare("weightcompliance", Qt::CaseInsensitive) || !type.compare("weightfrequency", Qt::CaseInsensitive)
                    || !type.compare("complianceindex", Qt::CaseInsensitive) || !type.compare("psddisplacement", Qt::CaseInsensitive)
                    || !type.compare("freqdisplacement", Qt::CaseInsensitive))
                {
                    //响应类型约束：解析与优化目标一致的*response子块
                    ReadStructOptionConstraintResponse(constraint, type, getKeyValue(currentline, "targetstep", "="));
                    continue;
                }

                currentline = _file.readLine().simplified();
            }
            else if (currentline.startsWith("*"))
            {
                break;
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }


    }

    //读取结构优化约束-响应类型子块，格式与 ReadStructOptionObjective 的*response子块保持一致
    void mIMxFile::ReadStructOptionConstraintResponse(MechStructOptionData::mOptimizationConstraintData* constraint, const QString& type, const QString& targetstep)
    {
        if (constraint == nullptr) return;
        int id = constraint->getId();

        if (!type.compare("compliance", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "柔度";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("stress", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "应力";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("thermalcompliance", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "热柔度";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("frequency", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "频率";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();
            constraint->_frequencyOrder = currentline;
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("temperature", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "节点温度";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();

            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            constraint->_node = { MxFunctions::PickDataType::Node, data };
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("displacement", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "位移";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();

            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            constraint->_node = { MxFunctions::PickDataType::Node, data };

            vector<int> dof;
            for (int i = 1; i < 7; i++)
            {
                int value = getQStringListIndexValue(_linelist, i).toInt();
                if (value > 0)
                {
                    dof.push_back(i);
                }
            }

            if (dof.size() == 1)
            {
                constraint->_nodeDofs = MxFunctions::mNumberToString(dof[0]);
            }
            else if (!dof.empty())
            {
                if (dof[0] > 3) constraint->_nodeDofs = "rot";
                else constraint->_nodeDofs = "disp";
            }

            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("weightfrequency", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "综合频率";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);

            double x, y;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;

                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                x = getQStringListIndexValue(_linelist, 0).toDouble();
                y = getQStringListIndexValue(_linelist, 1).toDouble();
                constraint->_order.push_back({ x,y });
            }
        }
        else if (!type.compare("weightcompliance", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "综合柔度";
            QString caseNmae; double value;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;

                _linelist = currentline.split(",");
                caseNmae = getQStringListIndexValue(_linelist, 0);
                value = getQStringListIndexValue(_linelist, 1).toDouble();

                if (caseNmae.isEmpty()) continue;
                caseNmae = ExistRenamePhysicalName(Physical_Case, caseNmae);
                int phyID = _modeldata->getPhyIDByName(Physical_Case, caseNmae);
                if (phyID == -1) continue;

                constraint->_caseToweight[phyID] = value;
            }
        }
        else if (!type.compare("complianceindex", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "柔度指数";
            currentline = _file.readLine().simplified();
            while (!_file.atEnd())
            {
                if (currentline.startsWith("*linearstep", Qt::CaseInsensitive))
                {
                    constraint->_isCompliance = true;
                    QString caseNmae; double value;
                    while (!_file.atEnd())
                    {
                        currentline = _file.readLine().simplified();
                        if (currentline.startsWith("*"))
                            break;

                        _linelist = currentline.split(",");
                        caseNmae = getQStringListIndexValue(_linelist, 0);
                        value = getQStringListIndexValue(_linelist, 1).toDouble();

                        if (caseNmae.isEmpty()) continue;
                        caseNmae = ExistRenamePhysicalName(Physical_Case, caseNmae);
                        int phyID = _modeldata->getPhyIDByName(Physical_Case, caseNmae);
                        if (phyID == -1) continue;

                        constraint->_caseToweight[phyID] = value;
                    }
                }
                else if (currentline.startsWith("*modestep", Qt::CaseInsensitive))
                {
                    constraint->_isFrequency = true;
                    QString name = getKeyValue(currentline, "name", "=");
                    appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, name);

                    double x, y;
                    while (!_file.atEnd())
                    {
                        currentline = _file.readLine().simplified();
                        if (currentline.startsWith("*"))
                            break;

                        if (currentline.isEmpty())
                            continue;

                        _linelist = currentline.split(",");
                        x = getQStringListIndexValue(_linelist, 0).toDouble();
                        y = getQStringListIndexValue(_linelist, 1).toDouble();
                        constraint->_order.push_back({ x,y });
                    }
                }
                else if (currentline.startsWith("*", Qt::CaseInsensitive))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }
        else if (!type.compare("psddisplacement", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "PSD位移";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);

            // 跳过 *node 关键字行，读取节点数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*node", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            constraint->_node = { MxFunctions::PickDataType::Node, data };

            vector<int> dof;
            for (int i = 1; i < 7; i++)
            {
                int value = getQStringListIndexValue(_linelist, i).toInt();
                if (value > 0)
                {
                    dof.push_back(i);
                }
            }
            if (dof.size() == 1)
            {
                constraint->_nodeDofs = MxFunctions::mNumberToString(dof[0]);
            }
            else if (!dof.empty())
            {
                if (dof[0] > 3) constraint->_nodeDofs = "rot";
                else constraint->_nodeDofs = "disp";
            }

            // 跳过 *freqpoint 关键字行，读取频率数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*freqpoint", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            QString freqText = currentline;
            bool isDouble = false;
            double freqValue = freqText.toDouble(&isDouble);
            if (isDouble)
            {
                constraint->_frequencyTargetType = "指定频率点";
                constraint->_specifiedFrequency = MxFunctions::mNumberToString(freqValue, 'g', 15);
            }
            else
            {
                if (!freqText.compare("MAX", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "最大响应点";
                else if (!freqText.compare("MIN", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "最小响应点";
                else if (!freqText.compare("AVG", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "平均";
                else if (!freqText.compare("SUM", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "求和";
                else if (!freqText.compare("RMS", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "RMS";
                else
                    constraint->_frequencyTargetType = freqText;
                constraint->_specifiedFrequency = "";
            }

            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("freqdisplacement", Qt::CaseInsensitive))
        {
            constraint->_constraintType = "频响位移";
            appendMapPhysicalStruct(Physical_OptimationConstraint, id, Physical_Case, targetstep);

            // 跳过 *node 关键字行，读取节点数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*node", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            constraint->_node = { MxFunctions::PickDataType::Node, data };

            vector<int> dof;
            for (int i = 1; i < 7; i++)
            {
                int value = getQStringListIndexValue(_linelist, i).toInt();
                if (value > 0)
                {
                    dof.push_back(i);
                }
            }
            if (dof.size() == 1)
            {
                constraint->_nodeDofs = MxFunctions::mNumberToString(dof[0]);
            }
            else if (!dof.empty())
            {
                if (dof[0] > 3) constraint->_nodeDofs = "rot";
                else constraint->_nodeDofs = "disp";
            }

            // 解析 ValueType
            QString valueType = getQStringListIndexValue(_linelist, 7);
            if (!valueType.compare("M", Qt::CaseInsensitive))
                constraint->_frfValueType = "幅值";
            else if (!valueType.compare("R", Qt::CaseInsensitive))
                constraint->_frfValueType = "实部";
            else if (!valueType.compare("I", Qt::CaseInsensitive))
                constraint->_frfValueType = "虚部";
            else
                constraint->_frfValueType = valueType;

            // 跳过 *freqpoint 关键字行，读取频率数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*freqpoint", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            QString freqText = currentline;
            bool isDouble = false;
            double freqValue = freqText.toDouble(&isDouble);
            if (isDouble)
            {
                constraint->_frequencyTargetType = "指定频率点";
                constraint->_specifiedFrequency = MxFunctions::mNumberToString(freqValue, 'g', 15);
            }
            else
            {
                if (!freqText.compare("MAX", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "最大响应点";
                else if (!freqText.compare("MIN", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "最小响应点";
                else if (!freqText.compare("AVG", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "平均";
                else if (!freqText.compare("SUM", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "求和";
                else if (!freqText.compare("RMS", Qt::CaseInsensitive))
                    constraint->_frequencyTargetType = "RMS";
                else
                    constraint->_frequencyTargetType = freqText;
                constraint->_specifiedFrequency = "";
            }

            currentline = _file.readLine().simplified();
        }
        else
        {
            currentline = _file.readLine().simplified();
        }
    }
    void mIMxFile::ReadStructOptionObjective()
    {
        MechStructOptionData::mStructOption* opt = _modeldata->getStructOptionBase();
        QString name = getKeyValue(currentline, "name", "=");
        QString type = getKeyValue(currentline, "type", "=");
        QString target = getKeyValue(currentline, "target", "=");
        MechStructOptionData::mOptimizationGoalData* goal = opt->_goalData;

        if (!type.compare("min", Qt::CaseInsensitive))
        {
            goal->_objectiveType = "最小化";
        }
        else if (!type.compare("max", Qt::CaseInsensitive))
        {
            goal->_objectiveType = "最大化";
        }
        int modelID = _modeldata->getModelDataID();

        currentline = _file.readLine().simplified();
        type = getKeyValue(currentline, "type", "=");
        QString targetstep = getKeyValue(currentline, "targetstep", "=");
        if (!type.compare("compliance", Qt::CaseInsensitive))
        {
            goal->_responseType = "柔度";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();
        }
        else  if (!type.compare("stress", Qt::CaseInsensitive))
        {
            goal->_responseType = "应力";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();
        }
        else  if (!type.compare("thermalcompliance", Qt::CaseInsensitive))
        {
            goal->_responseType = "热柔度";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("frequency", Qt::CaseInsensitive))
        {
            goal->_responseType = "频率";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();
            goal->_frequencyOrder = currentline;
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("temperature", Qt::CaseInsensitive))
        {
            goal->_responseType = "节点温度";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();

            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            goal->_node = { MxFunctions::PickDataType::Node, data };
            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("displacement", Qt::CaseInsensitive))
        {
            goal->_responseType = "位移";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);
            currentline = _file.readLine().simplified();

            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            goal->_node = { MxFunctions::PickDataType::Node, data };

            vector<int> dof;
            for (int i = 1; i < 7; i++)
            {
                int value = getQStringListIndexValue(_linelist, i).toInt();
                if (value > 0)
                {
                    dof.push_back(i);
                }
            }

            if (dof.size() == 1)
            {
                goal->_nodeDofs = MxFunctions::mNumberToString(dof[0]);
            }
            else
            {
                if (dof[0] > 3) goal->_nodeDofs = "rot";
                else goal->_nodeDofs = "disp";
            }

            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("weightfrequency", Qt::CaseInsensitive))
        {
            goal->_responseType = "综合频率";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);

            double x, y;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;

                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                x = getQStringListIndexValue(_linelist, 0).toDouble();
                y = getQStringListIndexValue(_linelist, 1).toDouble();
                goal->_order.push_back({ x,y });
            }
        }
        else if (!type.compare("weightcompliance", Qt::CaseInsensitive))
        {
            goal->_responseType = "综合柔度";
            QString caseNmae; double value;
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;

                _linelist = currentline.split(",");
                caseNmae = getQStringListIndexValue(_linelist, 0);
                value = getQStringListIndexValue(_linelist, 1).toDouble();

                if (caseNmae.isEmpty()) continue;
                caseNmae = ExistRenamePhysicalName(Physical_Case, caseNmae);
                int phyID = _modeldata->getPhyIDByName(Physical_Case, caseNmae);
                if (phyID == -1) continue;

                goal->_caseToweight[phyID] = value;
            }
        }
        else if (!type.compare("complianceindex", Qt::CaseInsensitive))
        {
            goal->_responseType = "柔度指数";
            currentline = _file.readLine().simplified();
            while (!_file.atEnd())
            {
                if (currentline.startsWith("*linearstep", Qt::CaseInsensitive))
                {
                    goal->_isCompliance = true;
                    QString caseNmae; double value;
                    while (!_file.atEnd())
                    {
                        currentline = _file.readLine().simplified();
                        if (currentline.startsWith("*"))
                            break;

                        _linelist = currentline.split(",");
                        caseNmae = getQStringListIndexValue(_linelist, 0);
                        value = getQStringListIndexValue(_linelist, 1).toDouble();

                        if (caseNmae.isEmpty()) continue;
                        caseNmae = ExistRenamePhysicalName(Physical_Case, caseNmae);
                        int phyID = _modeldata->getPhyIDByName(Physical_Case, caseNmae);
                        if (phyID == -1) continue;

                        goal->_caseToweight[phyID] = value;
                    }
                }
                else  if (currentline.startsWith("*modestep", Qt::CaseInsensitive))
                {
                    goal->_isFrequency = true;
                    name = getKeyValue(currentline, "name", "=");
                    appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, name);

                    double x, y;
                    while (!_file.atEnd())
                    {
                        currentline = _file.readLine().simplified();
                        if (currentline.startsWith("*"))
                            break;

                        if (currentline.isEmpty())
                            continue;

                        _linelist = currentline.split(",");
                        x = getQStringListIndexValue(_linelist, 0).toDouble();
                        y = getQStringListIndexValue(_linelist, 1).toDouble();
                        goal->_order.push_back({ x,y });
                    }
                }
                else  if (currentline.startsWith("*", Qt::CaseInsensitive))
                {
                    break;
                }
                else
                {
                    currentline = _file.readLine().simplified();
                }
            }
        }
        else if (!type.compare("psddisplacement", Qt::CaseInsensitive))
        {
            goal->_responseType = "PSD位移";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);

            // 跳过 *node 关键字行，读取节点数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*node", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            goal->_node = { MxFunctions::PickDataType::Node, data };

            vector<int> dof;
            for (int i = 1; i < 7; i++)
            {
                int value = getQStringListIndexValue(_linelist, i).toInt();
                if (value > 0)
                {
                    dof.push_back(i);
                }
            }
            if (dof.size() == 1)
            {
                goal->_nodeDofs = MxFunctions::mNumberToString(dof[0]);
            }
            else if (!dof.empty())
            {
                if (dof[0] > 3) goal->_nodeDofs = "rot";
                else goal->_nodeDofs = "disp";
            }

            // 跳过 *freqpoint 关键字行，读取频率数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*freqpoint", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            QString freqText = currentline;
            bool isDouble = false;
            double freqValue = freqText.toDouble(&isDouble);
            if (isDouble)
            {
                goal->_frequencyTargetType = "指定频率点";
                goal->_specifiedFrequency = MxFunctions::mNumberToString(freqValue, 'g', 15);
            }
            else
            {
                if (!freqText.compare("MAX", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "最大响应点";
                else if (!freqText.compare("MIN", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "最小响应点";
                else if (!freqText.compare("AVG", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "平均";
                else if (!freqText.compare("SUM", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "求和";
                else if (!freqText.compare("RMS", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "RMS";
                else
                    goal->_frequencyTargetType = freqText;
                goal->_specifiedFrequency = "";
            }

            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("freqdisplacement", Qt::CaseInsensitive))
        {
            goal->_responseType = "频响位移";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);

            // 跳过 *node 关键字行，读取节点数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*node", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            goal->_node = { MxFunctions::PickDataType::Node, data };

            vector<int> dof;
            for (int i = 1; i < 7; i++)
            {
                int value = getQStringListIndexValue(_linelist, i).toInt();
                if (value > 0)
                {
                    dof.push_back(i);
                }
            }
            if (dof.size() == 1)
            {
                goal->_nodeDofs = MxFunctions::mNumberToString(dof[0]);
            }
            else if (!dof.empty())
            {
                if (dof[0] > 3) goal->_nodeDofs = "rot";
                else goal->_nodeDofs = "disp";
            }

            // 解析 ValueType
            QString valueType = getQStringListIndexValue(_linelist, 7);
            if (!valueType.compare("M", Qt::CaseInsensitive))
                goal->_frfValueType = "幅值";
            else if (!valueType.compare("R", Qt::CaseInsensitive))
                goal->_frfValueType = "实部";
            else if (!valueType.compare("I", Qt::CaseInsensitive))
                goal->_frfValueType = "虚部";
            else
                goal->_frfValueType = valueType;

            // 跳过 *freqpoint 关键字行，读取频率数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*freqpoint", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            QString freqText = currentline;
            bool isDouble = false;
            double freqValue = freqText.toDouble(&isDouble);
            if (isDouble)
            {
                goal->_frequencyTargetType = "指定频率点";
                goal->_specifiedFrequency = MxFunctions::mNumberToString(freqValue, 'g', 15);
            }
            else
            {
                if (!freqText.compare("MAX", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "最大响应点";
                else if (!freqText.compare("MIN", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "最小响应点";
                else if (!freqText.compare("AVG", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "平均";
                else if (!freqText.compare("SUM", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "求和";
                else if (!freqText.compare("RMS", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "RMS";
                else
                    goal->_frequencyTargetType = freqText;
                goal->_specifiedFrequency = "";
            }

            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("psddisplacement", Qt::CaseInsensitive))
        {
            goal->_responseType = "PSD位移";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);

            // 跳过 *node 关键字行，读取节点数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*node", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            goal->_node = { MxFunctions::PickDataType::Node, data };

            vector<int> dof;
            for (int i = 1; i < 7; i++)
            {
                int value = getQStringListIndexValue(_linelist, i).toInt();
                if (value > 0)
                {
                    dof.push_back(i);
                }
            }
            if (dof.size() == 1)
            {
                // 单个自由度：读成对应数字（如 1,0,0,0,0,0→"1"；0,1,0,0,0,0→"2"）
                goal->_nodeDofs = MxFunctions::mNumberToString(dof[0]);
            }
            else if (!dof.empty())
            {
                // 多个自由度：读成 disp/rot
                if (dof[0] > 3) goal->_nodeDofs = "rot";
                else goal->_nodeDofs = "disp";
            }

            // 跳过 *freqpoint 关键字行，读取频率数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*freqpoint", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            QString freqText = currentline;
            bool isDouble = false;
            double freqValue = freqText.toDouble(&isDouble);
            if (isDouble)
            {
                goal->_frequencyTargetType = "指定频率点";
                goal->_specifiedFrequency = MxFunctions::mNumberToString(freqValue, 'g', 15);
            }
            else
            {
                if (!freqText.compare("MAX", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "最大响应点";
                else if (!freqText.compare("MIN", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "最小响应点";
                else if (!freqText.compare("AVG", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "平均";
                else if (!freqText.compare("SUM", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "求和";
                else if (!freqText.compare("RMS", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "RMS";
                else
                    goal->_frequencyTargetType = freqText;
                goal->_specifiedFrequency = "";
            }

            currentline = _file.readLine().simplified();
        }
        else if (!type.compare("freqdisplacement", Qt::CaseInsensitive))
        {
            goal->_responseType = "频响位移";
            appendMapPhysicalStruct(Physical_OptimationGoal, modelID, Physical_Case, targetstep);

            // 跳过 *node 关键字行，读取节点数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*node", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            RenderPickResultData data;
            data.result_id = getQStringListIndexValue(_linelist, 0).toInt() + _lastMaxNodeID;
            goal->_node = { MxFunctions::PickDataType::Node, data };

            vector<int> dof;
            for (int i = 1; i < 7; i++)
            {
                int value = getQStringListIndexValue(_linelist, i).toInt();
                if (value > 0)
                {
                    dof.push_back(i);
                }
            }
            if (dof.size() == 1)
            {
                // 单个自由度：读成对应数字（如 1,0,0,0,0,0→"1"；0,1,0,0,0,0→"2"）
                goal->_nodeDofs = MxFunctions::mNumberToString(dof[0]);
            }
            else if (!dof.empty())
            {
                // 多个自由度：读成 disp/rot
                if (dof[0] > 3) goal->_nodeDofs = "rot";
                else goal->_nodeDofs = "disp";
            }

            // 解析 ValueType
            QString valueType = getQStringListIndexValue(_linelist, 7);
            if (!valueType.compare("M", Qt::CaseInsensitive))
                goal->_frfValueType = "幅值";
            else if (!valueType.compare("R", Qt::CaseInsensitive))
                goal->_frfValueType = "实部";
            else if (!valueType.compare("I", Qt::CaseInsensitive))
                goal->_frfValueType = "虚部";
            else
                goal->_frfValueType = valueType;

            // 跳过 *freqpoint 关键字行，读取频率数据行
            currentline = _file.readLine().simplified();
            while (!_file.atEnd() && currentline.compare("*freqpoint", Qt::CaseInsensitive) != 0)
                currentline = _file.readLine().simplified();

            currentline = _file.readLine().simplified();
            QString freqText = currentline;
            bool isDouble = false;
            double freqValue = freqText.toDouble(&isDouble);
            if (isDouble)
            {
                goal->_frequencyTargetType = "指定频率点";
                goal->_specifiedFrequency = MxFunctions::mNumberToString(freqValue, 'g', 15);
            }
            else
            {
                if (!freqText.compare("MAX", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "最大响应点";
                else if (!freqText.compare("MIN", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "最小响应点";
                else if (!freqText.compare("AVG", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "平均";
                else if (!freqText.compare("SUM", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "求和";
                else if (!freqText.compare("RMS", Qt::CaseInsensitive))
                    goal->_frequencyTargetType = "RMS";
                else
                    goal->_frequencyTargetType = freqText;
                goal->_specifiedFrequency = "";
            }

            currentline = _file.readLine().simplified();
        }

    }
    void mIMxFile::readGenericAssignData()
    {
        auto assign = _modeldata->getGlobalParam()->getGenericAssignData();
        if (!assign) assign = _modeldata->getGlobalParam()->createGenericAssignData();
        assign->_filePath.clear();

        // ========== 读取*assign块：超单元文件路径 ==========
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*"))
                break;

            if (currentline.isEmpty())
                continue;

            QString filepath = currentline.replace("\"", "");
            QFileInfo fileInfo(filepath);
            if (!fileInfo.isAbsolute())
            {
                QFileInfo baseFileInfo(_filename);
                filepath = baseFileInfo.absolutePath() + "/" + filepath;
            }
            assign->_filePath.push_back(filepath);
        }
    }
    void mIMxFile::readGenericParamData()
    {
        QString method = getKeyValue(currentline, "method", "=");
        QString name = getKeyValue(currentline, "name", "=");
        name = ExistRenamePhysicalName(Physical_GenericParam, name);

        int id = _modeldata->getGlobalParam()->createGenericParamID();
        auto param = new mGenericParamData(id, name);
        _modeldata->getGlobalParam()->addGenericParamData(param);

        param->_method = method;

        currentline = _file.readLine().simplified();
        if (!currentline.startsWith("*", Qt::CaseInsensitive))
        {
            _linelist = currentline.split(",");
            param->_upFrequency = getQStringListIndexValue(_linelist, 0);
            param->_order = getQStringListIndexValue(_linelist, 1);
            param->_startID = QString::number(getQStringListIndexValue(_linelist, 2).toInt() + _lastMaxNodeID);

            currentline = _file.readLine().simplified();
        }
    }
    void mIMxFile::readParam()
    {
        while (!_file.atEnd())
        {
            currentline = _file.readLine().simplified();
            if (currentline.startsWith("*"))
                break;

            if (currentline.isEmpty())
                continue;

            if (currentline.contains("DMIGNAME", Qt::CaseInsensitive))
            {
                auto matrix = _modeldata->getGlobalParam()->getGenericMatrixData();
                if (!matrix) matrix = _modeldata->getGlobalParam()->createGenericMatrixData();
                QString DMIGNAME = getKeyValue(currentline, "DMIGNAME", "=");
                matrix->_matrixName = DMIGNAME;
            }
            else if (currentline.contains("K2GG", Qt::CaseInsensitive))
            {
                auto assign = _modeldata->getGlobalParam()->getGenericAssignData();
                if (!assign) assign = _modeldata->getGlobalParam()->createGenericAssignData();

                QString stiffnessValue;
                QStringList list = currentline.split("=");
                if (list.size() >= 2)
                    stiffnessValue = list[1];
                // 刚度矩阵
                if (!stiffnessValue.isEmpty())
                {
                    assign->_stiffnessMatrixNames = stiffnessValue.trimmed();
                    assign->_stiffnessMatrixEnabled = true;
                }
            }
            else if (currentline.contains("M2GG", Qt::CaseInsensitive))
            {
                auto assign = _modeldata->getGlobalParam()->getGenericAssignData();
                if (!assign) assign = _modeldata->getGlobalParam()->createGenericAssignData();
                QString massValue;
                QStringList list = currentline.split("=");
                if (list.size() >= 2)
                    massValue = list[1];
                // 质量矩阵
                if (!massValue.isEmpty())
                {
                    assign->_massMatrixNames = massValue.trimmed();
                    assign->_massMatrixEnabled = true;
                }
            }
            else if (currentline.contains("B2GG", Qt::CaseInsensitive))
            {
                auto assign = _modeldata->getGlobalParam()->getGenericAssignData();
                if (!assign) assign = _modeldata->getGlobalParam()->createGenericAssignData();
                QString dampingValue;
                QStringList list = currentline.split("=");
                if (list.size() >= 2)
                    dampingValue = list[1];
                if (!dampingValue.isEmpty())
                {
                    assign->_viscousDampingMatrixNames = dampingValue.trimmed();
                    assign->_viscousDampingMatrixEnabled = true;
                }
            }
            else if (currentline.contains("K42GG", Qt::CaseInsensitive))
            {
                auto assign = _modeldata->getGlobalParam()->getGenericAssignData();
                if (!assign) assign = _modeldata->getGlobalParam()->createGenericAssignData();
                QString materialValue;
                QStringList list = currentline.split("=");
                if (list.size() >= 2)
                    materialValue = list[1];

                // 材料矩阵
                if (!materialValue.isEmpty())
                {
                    assign->_materialMatrixNames = materialValue.trimmed();
                    assign->_materialMatrixEnabled = true;
                }
            }
        }
    }
    void mIMxFile::readASCouplingSearchData()
    {
        auto searchData = _modeldata->getGlobalParam()->getASCouplingSearchData();
        if (!searchData) searchData = _modeldata->getGlobalParam()->createASCouplingSearchData();

        // 解析 *fsi_pair 行
        QString autosearch = getKeyValue(currentline, "autosearch", "=");
        QString type = getKeyValue(currentline, "type", "=");

        searchData->setAutosearch(autosearch.compare("off", Qt::CaseInsensitive) != 0);
        searchData->setMainSurface(type);
        currentline = _file.readLine().simplified();

        // 读取可选子块
        while (!_file.atEnd())
        {
            if (!currentline.compare("*angle", Qt::CaseInsensitive))
            {
                searchData->setAngelChecked(true);
                currentline = _file.readLine().simplified();
                searchData->setSearchAngel(currentline);
            }
            else if (!currentline.compare("*normal", Qt::CaseInsensitive))
            {
                searchData->setOutChecked(true);
                currentline = _file.readLine().simplified();
                searchData->setOuterScale(currentline);
            }
            else if (!currentline.compare("*intol", Qt::CaseInsensitive))
            {
                searchData->setInnerChecked(true);
                currentline = _file.readLine().simplified();
                searchData->setInnerScale(currentline);
            }
            else if (!currentline.compare("*skneps", Qt::CaseInsensitive))
            {
                searchData->setHorizontalChecked(true);
                currentline = _file.readLine().simplified();
                searchData->setHorizontalScale(currentline);
            }
            else if (!currentline.compare("*skneps", Qt::CaseInsensitive))
            {
                searchData->setHorizontalChecked(true);
                currentline = _file.readLine().simplified();
                searchData->setHorizontalScale(currentline);
            }
            else if (!currentline.compare("*aabb", Qt::CaseInsensitive))
            {
                searchData->setBoundingBoxChecked(true);
                currentline = _file.readLine().simplified();
                searchData->setBoundingBoxScale(currentline);
            }
            else if (currentline.startsWith("*", Qt::CaseInsensitive))
            {
                break;
            }
            else
            {
                currentline = _file.readLine().simplified();
            }
        }
    }

    void mIMxFile::readFunctionData()
    {
        QString type = getKeyValue(currentline, "type", "=");
        QString axisx = getKeyValue(currentline, "axisx", "=");
        QString axisy = getKeyValue(currentline, "axisy", "=");
        QString extend = getKeyValue(currentline, "extend", "=");
        QString name = getKeyValue(currentline, "name", "=");
        QString x1 = getKeyValue(currentline, "x1", "=");
        QString x2 = getKeyValue(currentline, "x2", "=");
        name = ExistRenamePhysicalName(Physical_Function, name);

        int funcID = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).createID();
        MechFunctionData::mFunctionBase* func = new MechFunctionData::mFunctionBase(funcID, name);
        _modeldata->addPhysicalField<MechFunctionData::mFunctionBase>(func);

        if (!type.compare("function1", Qt::CaseInsensitive))
        {
            func->setFunctionType(5);
            MechFunctionData::TableCol& data = func->table();
            if (!axisx.compare("linear", Qt::CaseInsensitive))
            {
                data.xAxisType = 0;
            }
            else if (!axisx.compare("log", Qt::CaseInsensitive))
            {
                data.xAxisType = 1;
            }

            if (!axisy.compare("linear", Qt::CaseInsensitive))
            {
                data.yAxisType = 0;
            }
            else if (!axisy.compare("log", Qt::CaseInsensitive))
            {
                data.yAxisType = 1;
            }

            data.extrapMode = extend.toInt();

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;

                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                data.x.push_back(getQStringListIndexValue(_linelist, 0).toDouble());
                data.y.push_back(getQStringListIndexValue(_linelist, 1).toDouble());
            }
        }
        else  if (!type.compare("function2", Qt::CaseInsensitive))
        {
            func->setFunctionType(6);

            MechFunctionData::TableCol& data = func->table();
            data.extrapMode = extend.toInt();
            data.x1 = x1.toDouble();

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;

                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                data.x.push_back(getQStringListIndexValue(_linelist, 0).toDouble());
                data.y.push_back(getQStringListIndexValue(_linelist, 1).toDouble());
            }
        }
        else  if (!type.compare("function3", Qt::CaseInsensitive))
        {
            func->setFunctionType(7);

            MechFunctionData::TableCol& data = func->table();
            data.extrapMode = extend.toInt();
            data.x1 = x1.toDouble();
            data.x2 = x2.toDouble();

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;

                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                data.x.push_back(getQStringListIndexValue(_linelist, 0).toDouble());
                data.y.push_back(getQStringListIndexValue(_linelist, 1).toDouble());
            }
        }
        else  if (!type.compare("function4", Qt::CaseInsensitive))
        {
            func->setFunctionType(8);

            MechFunctionData::TableCol& data = func->table();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            data.x1 = getQStringListIndexValue(_linelist, 0).toDouble();
            data.x2 = getQStringListIndexValue(_linelist, 1).toDouble();
            data.x3 = getQStringListIndexValue(_linelist, 2).toDouble();
            data.x4 = getQStringListIndexValue(_linelist, 3).toDouble();

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;

                if (currentline.isEmpty())
                    continue;

                _linelist = currentline.split(",");
                for (int i = 0; i < _linelist.size(); i++)
                {
                    data.x.push_back(getQStringListIndexValue(_linelist, i).toDouble());
                }
            }
            data.y.resize(data.x.size());
        }
        else  if (!type.compare("log", Qt::CaseInsensitive) || !type.compare("linear", Qt::CaseInsensitive)
            || !type.compare("smooth", Qt::CaseInsensitive) || !type.compare("step", Qt::CaseInsensitive))
        {
            func->setFunctionType(2);
            MechFunctionData::InterpCol& data = func->interp();

            if (!type.compare("log", Qt::CaseInsensitive))
            {
                data.interpMode = 4;
            }
            else if (!type.compare("linear", Qt::CaseInsensitive))
            {
                data.interpMode = 1;
            }
            else if (!type.compare("smooth", Qt::CaseInsensitive))
            {
                data.interpMode = 5;   // 光滑
            }
            else if (!type.compare("step", Qt::CaseInsensitive))
            {
                data.interpMode = 6;   // 阶跃
            }

            if (!extend.compare("const", Qt::CaseInsensitive))
            {
                data.extrapMode = 0;
            }
            else  if (!extend.compare("linear", Qt::CaseInsensitive))
            {
                data.extrapMode = 1;
            }

            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty())
                    continue;
                _linelist = currentline.split(",");
                data.x.push_back(getQStringListIndexValue(_linelist, 0).toDouble());
                data.y.push_back(getQStringListIndexValue(_linelist, 1).toDouble());
            }
        }
        else  if (!type.compare("periodic", Qt::CaseInsensitive))
        {
            // *function, type = periodic, name = "xxx"
            // N, f, t0, A0
            // cos1, sin1
            // cos2, sin2 ...
            func->setFunctionType(9);
            MechFunctionData::PeriodicCol& data = func->periodic();

            // 第一行参数：N, f, t0, A0
            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            data.order = getQStringListIndexValue(_linelist, 0).toInt();
            data.freq = getQStringListIndexValue(_linelist, 1).toDouble();
            data.t0 = getQStringListIndexValue(_linelist, 2).toDouble();
            data.A0 = getQStringListIndexValue(_linelist, 3).toDouble();

            // 后续 cos/sin 系数行
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty())
                    continue;
                _linelist = currentline.split(",");
                data.A.push_back(getQStringListIndexValue(_linelist, 0).toDouble());
                data.B.push_back(getQStringListIndexValue(_linelist, 1).toDouble());
            }
        }
        else  if (!type.compare("modulated", Qt::CaseInsensitive))
        {
            // *function, type = modulated, name = "xxx"
            // A0, A, t0, f1, f2
            func->setFunctionType(10);
            MechFunctionData::ModulatedCol& data = func->modulated();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            data.A0 = getQStringListIndexValue(_linelist, 0).toDouble();
            data.A = getQStringListIndexValue(_linelist, 1).toDouble();
            data.t0 = getQStringListIndexValue(_linelist, 2).toDouble();
            data.f1 = getQStringListIndexValue(_linelist, 3).toDouble();
            data.f2 = getQStringListIndexValue(_linelist, 4).toDouble();

            // 读取到下一个关键字，保持游标一致
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty())
                    continue;
            }
        }
        else  if (!type.compare("decay", Qt::CaseInsensitive))
        {
            // *function, type = decay, name = "xxx"
            // A0, Am, t0, td
            func->setFunctionType(11);
            MechFunctionData::ExpDecayCol& data = func->expDecay();

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            data.A0 = getQStringListIndexValue(_linelist, 0).toDouble();
            data.Am = getQStringListIndexValue(_linelist, 1).toDouble();
            data.t0 = getQStringListIndexValue(_linelist, 2).toDouble();
            data.td = getQStringListIndexValue(_linelist, 3).toDouble();

            // 读取到下一个关键字，保持游标一致
            while (!_file.atEnd())
            {
                currentline = _file.readLine().simplified();
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty())
                    continue;
            }
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("函数 %1  类型不支持！").arg(name));
            currentline = _file.readLine().simplified();
        }

    }
    QString mIMxFile::ExistRenamePhysicalName(MxFunctions::PhysicalFieldType type, const QString& name)
    {
        if (_readPhyName[type].find(name) != _readPhyName[type].end())
        {
            return _readPhyName[type].value(name);
        }

        if (!_modeldata->isExistName(type, name, true))
        {
            _readPhyName[type][name] = name;
            return name;
        }

        int i = 1;
        while (1)
        {
            QString Rename = QString("%1_%2").arg(name).arg(i++);
            if (!_modeldata->isExistName(type, Rename, true))
            {
                _readPhyName[type][name] = Rename;
                return Rename;
            }
        }
    }
    void mIMxFile::readTPContriData()
    {
        // 读取第一行：*path, type = xxx, name = "xxx"
        QString respType = getKeyValue(currentline, "type", "=");
        QString pathName = getKeyValue(currentline, "name", "=");
        pathName.remove('"');

        auto tpData = _modeldata->getGlobalParam()->createTPContriData();
        tpData->setName(pathName);
        tpData->_respType = respType;

        int tpId = tpData->getId();

        // 读取第二行：集合名称列表
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");

        QString conptSetName = getQStringListIndexValue(_linelist, 0).trimmed();
        QString ridSetName = getQStringListIndexValue(_linelist, 1).trimmed();
        QString conelSetName = getQStringListIndexValue(_linelist, 2).trimmed();
        QString conrelSetName = getQStringListIndexValue(_linelist, 3).trimmed();
        QString convolSetName = getQStringListIndexValue(_linelist, 4).trimmed();

        // 建立映射关系
        appendMapPhysicalStruct(Physical_TPContri, tpId, Physical_Set, conptSetName, MapRelation_ConptSet);
        appendMapPhysicalStruct(Physical_TPContri, tpId, Physical_Set, ridSetName, MapRelation_ResponseGridSet);
        appendMapPhysicalStruct(Physical_TPContri, tpId, Physical_Set, conelSetName, MapRelation_ConelSet);
        if (!conrelSetName.isEmpty())
        {
            appendMapPhysicalStruct(Physical_TPContri, tpId, Physical_Set, conrelSetName, MapRelation_ConrelSet);
        }
        if (!convolSetName.isEmpty())
        {
            appendMapPhysicalStruct(Physical_TPContri, tpId, Physical_Set, convolSetName, MapRelation_ConvolSet);
        }

        currentline = _file.readLine().simplified();
    }

    void mIMxFile::appendMapPhysicalStruct(MxFunctions::PhysicalFieldType phyType, int id, MxFunctions::PhysicalFieldType resultType, QString name, MxFunctions::MapRelationType mapType)
    {
        if (name.isEmpty())
            return;

        name = ExistRenamePhysicalName(resultType, name);
        int phyID2 = _modeldata->getPhyIDByName(resultType, name);
        if (phyID2 == -1)
        {
            _mapPhysicalStruct[phyType][id][resultType][mapType] = name;
        }
        else
        {
            _modeldata->getMapRelationManager().appendMapRelation({ phyType, id }, { resultType, phyID2 }, mapType);
        }
    }
    void mIMxFile::appendMapPhysicalTable(MxFunctions::PhysicalFieldType phyType, int id1, MxFunctions::PhysicalFieldType type2, std::vector<QString> nameList)
    {
        if (nameList.empty())
        {
            return;
        }
        _mapVecPhysical[phyType][id1][type2] = nameList;
    }

    void mIMxFile::contactMap()
    {
        MechMapRelation::mMapRelationManager& manager = _modeldata->getMapRelationManager();
        for (auto it = _contactinterference_hash.begin(); it != _contactinterference_hash.cend(); ++it)
        {
            if (_contact_hash.find(it.key()) != _contact_hash.cend())
            {
                manager.appendMapRelation({ Physical_Contact, it.value() }, { Physical_Contact, _contact_hash[it.key()] });
            }
        }
    }

    void mIMxFile::nodeCoordMap()
    {
        for (auto it : _nodeCoordinateData)
        {
            int NodeId = _meshData->_nodedata.nodeidmap.value(it.first);
            double* xyz = _meshData->_nodedata.xyz;
            double x = xyz[NodeId * 3];
            double y = xyz[NodeId * 3 + 1];
            double z = xyz[NodeId * 3 + 2];

            QString referCoord = it.second.first;
            QString showCoord = it.second.second;
            int referID = _modeldata->getPhyIDByName(Physical_Coordinate, referCoord);
            int showID = _modeldata->getPhyIDByName(Physical_Coordinate, showCoord);

            if (!referCoord.isEmpty())
            {
                MechCoordinateData::mCoordinateBase* coorBase = _modeldata->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(referID);
                if (coorBase != nullptr)
                {
                    // 缓存轴数据
                    bool success; QString msg; QVector3D origin;
                    const QVector<QVector3D> xyzAxis = coorBase->getXYZAxis(success, msg, origin);

                    QVector3D zAxisPoint(origin.x() + xyzAxis[2].x(), origin.y() + xyzAxis[2].y(), origin.z() + xyzAxis[2].z());
                    QVector3D refPoint(origin.x() + xyzAxis[0].x(), origin.y() + xyzAxis[0].y(), origin.z() + xyzAxis[0].z());

                    if (coorBase->getCoordinateType() == Cylindrical)
                    {
                        //单位制换算：柱坐标x=r,z=h为长度；y=θ(度)为文件格式固定语义不换算
                        x = mUnitIoCodec::decode(MxFunctions::Unit::Length, x);
                        z = mUnitIoCodec::decode(MxFunctions::Unit::Length, z);

                        QVector3D e_z = (zAxisPoint - origin).normalized();
                        QVector3D v_ref = refPoint - origin;
                        QVector3D e_x = (v_ref - e_z * QVector3D::dotProduct(v_ref, e_z)).normalized();
                        QVector3D e_y = QVector3D::crossProduct(e_z, e_x);

                        // (x, y, z) 为柱坐标 (r, theta, h)，theta 为角度，需转换为弧度后计算
                        double r = x;
                        double theta = y * 3.14159265358979323846 / 180.0;
                        double h = z;
                        QVector3D point = origin + e_x * (r * std::cos(theta)) + e_y * (r * std::sin(theta)) + e_z * h;
                        x = point.x();
                        y = point.y();
                        z = point.z();
                    }
                    else
                    {
                        //单位制换算：参考直角坐标系下三分量均为长度
                        x = mUnitIoCodec::decode(MxFunctions::Unit::Length, x);
                        y = mUnitIoCodec::decode(MxFunctions::Unit::Length, y);
                        z = mUnitIoCodec::decode(MxFunctions::Unit::Length, z);

                        //Edge11 = {x2-x1,y2-y1,z2-z1}
                        //Edge12 = {x3-x1,y3-y1,z3-z1}
                        QVector3D Edge11 = xyzAxis[2];
                        QVector3D Edge12 = xyzAxis[0];
                        QVector3D Edge13;
                        QVector3D Edge14;

                        //Edge13= Edge11 × Edge12
                        Edge13 = QVector3D::crossProduct(QVector3D(Edge11[0], Edge11[1], Edge11[2]), QVector3D(Edge12[0], Edge12[1], Edge12[2]));
                        Edge14 = QVector3D::crossProduct(QVector3D(Edge13[0], Edge13[1], Edge13[2]), QVector3D(Edge11[0], Edge11[1], Edge11[2]));
                        Edge11.normalize();
                        Edge12.normalize();
                        Edge13.normalize();
                        Edge14.normalize();

                        double x1 = Edge14[0] * x + Edge13[0] * y + Edge11[0] * z + origin.x();
                        double y1 = Edge14[1] * x + Edge13[1] * y + Edge11[1] * z + origin.y();
                        double z1 = Edge14[2] * x + Edge13[2] * y + Edge11[2] * z + origin.z();
                        x = x1;
                        y = y1;
                        z = z1;

                    }

                    xyz[NodeId * 3] = x;
                    xyz[NodeId * 3 + 1] = y;
                    xyz[NodeId * 3 + 2] = z;

                }
            }
            else
            {
                //无参考坐标系（仅有显示坐标系）：节点值为全局坐标（长度），文件值 → 内部mm-t-N-s
                x = mUnitIoCodec::decode(MxFunctions::Unit::Length, x);
                y = mUnitIoCodec::decode(MxFunctions::Unit::Length, y);
                z = mUnitIoCodec::decode(MxFunctions::Unit::Length, z);
                xyz[NodeId * 3] = x;
                xyz[NodeId * 3 + 1] = y;
                xyz[NodeId * 3 + 2] = z;
            }

            _modeldata->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).addNodeCoordinate(it.first, referID, showID);
        }
    }

    void mIMxFile::readModalContriData(int caseID, MechGlobalParamData::mModalContriData* data)
    {
        if (!data) return;

        // 读取 *mcf 关键字行中的参数
        QString setName = getKeyValue(currentline, "set", "=");
        QString type = getKeyValue(currentline, "type", "=");
        QString frequencySetName = getKeyValue(currentline, "frequency", "=");
        QString sort = getKeyValue(currentline, "sort", "=");
        QString peakout = getKeyValue(currentline, "peakout", "=");

        // 建立映射关系
        appendMapPhysicalStruct(Physical_Case, caseID, Physical_ModalContri, data->getName());
        appendMapPhysicalStruct(Physical_ModalContri, data->getId(), Physical_Set, setName, MapRelation_OutSet);
        appendMapPhysicalStruct(Physical_ModalContri, data->getId(), Physical_Set, frequencySetName, MapRelation_FrequencySet);

        // 赋值给数据对象
        data->_outPutType = type;
        data->_SortType = sort;
        if (!peakout.compare("on", Qt::CaseInsensitive))
        {
            data->_checkBoxPkScr = true;
        }

        // 读取第二行：结果类型、过滤分数、输出阶数
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        data->_ResultType = getQStringListIndexValue(_linelist, 0);
        data->_fltScore = getQStringListIndexValue(_linelist, 1);
        data->_OutOrd = getQStringListIndexValue(_linelist, 2);

        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readNodeContriData(int caseID, MechGlobalParamData::mNodeContriData* data)
    {
        if (!data) return;

        // 读取 *pfgrid 关键字行中的参数
        QString gridSetName = getKeyValue(currentline, "grids", "=");
        QString gridfSetName = getKeyValue(currentline, "gridf", "=");
        QString freqSetName = getKeyValue(currentline, "solution", "=");
        QString rpcutoff = getKeyValue(currentline, "rpcutoff", "=");
        QString rpdbacut = getKeyValue(currentline, "rpdbacut", "=");
        QString contour = getKeyValue(currentline, "contour", "=");
        QString peakout = getKeyValue(currentline, "peakout", "=");

        // 建立映射关系
        appendMapPhysicalStruct(Physical_Case, caseID, Physical_NodeContri, data->getName());
        appendMapPhysicalStruct(Physical_NodeContri, data->getId(), Physical_Set, gridSetName, MapRelation_GridSet);
        appendMapPhysicalStruct(Physical_NodeContri, data->getId(), Physical_Set, gridfSetName, MapRelation_GridfSet);
        appendMapPhysicalStruct(Physical_NodeContri, data->getId(), Physical_Set, freqSetName, MapRelation_FrequencySet);

        // 赋值给数据对象
        if (!rpcutoff.isEmpty())
        {
            data->_rpTruncType = "帕斯卡";
            data->_rpTruncValue = rpcutoff;
        }
        else if (!rpdbacut.isEmpty())
        {
            data->_rpTruncType = "分贝计权";
            data->_rpTruncValue = rpdbacut;
        }
        if (!contour.compare("on", Qt::CaseInsensitive))
        {
            data->_checkBoxCavityProj = true;
        }
        if (!peakout.compare("on", Qt::CaseInsensitive))
        {
            data->_checkBoxPkScr = true;
        }

        // 读取第二行：响应点集合（可选行，可与 peakout 共存：未设置时整行不写，
        // 下一行直接是 * 开头的关键字行；空串或空行同样视为未设置）
        currentline = _file.readLine().simplified();
        if (!currentline.startsWith("*"))
        {
            QString respSetName = currentline;
            respSetName.remove('"');
            appendMapPhysicalStruct(Physical_NodeContri, data->getId(), Physical_Set, respSetName, MapRelation_ResponseGridSet);

            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readPanelContriData(int caseID, MechGlobalParamData::mPanelContriData* data)
    {
        if (!data) return;

        // 读取 *pfpanel 关键字行中的参数
        QString freqSetName = getKeyValue(currentline, "frequency", "=");
        QString type = getKeyValue(currentline, "type", "=");
        QString peakout = getKeyValue(currentline, "peakout", "=");

        // 建立映射关系
        appendMapPhysicalStruct(Physical_Case, caseID, Physical_PanelContri, data->getName());
        appendMapPhysicalStruct(Physical_PanelContri, data->getId(), Physical_Set, freqSetName, MapRelation_FrequencySet);

        // 赋值给数据对象
        if (!type.compare("phase", Qt::CaseInsensitive))
        {
            data->_outPutType = "幅值";
        }
        else
        {
            data->_outPutType = "";
        }
        if (!peakout.compare("on", Qt::CaseInsensitive))
        {
            data->_checkBoxPkScr = true;
        }

        // 读取第二行：响应点集合（可选行，可与 peakout 共存：未设置时整行不写，
        // 下一行直接是 * 开头的关键字行；空串或空行同样视为未设置）
        currentline = _file.readLine().simplified();
        if (!currentline.startsWith("*"))
        {
            QString respSetName = currentline;
            respSetName.remove('"');
            appendMapPhysicalStruct(Physical_PanelContri, data->getId(), Physical_Set, respSetName, MapRelation_ResponseGridSet);

            currentline = _file.readLine().simplified();
        }
    }

    void mIMxFile::readERPData()
    {
        // 等效辐射功率全局工况公用一份数据，若已存在则跳过，避免重复存储
        if (_modeldata->getGlobalParam()->getERPData())
        {
            currentline = _file.readLine().simplified();
            currentline = _file.readLine().simplified();
            return;
        }

        mERPData* erpData = _modeldata->getGlobalParam()->createERPData();
        currentline = _file.readLine().simplified();
        _linelist = currentline.split(",");
        erpData->_erprho = getQStringListIndexValue(_linelist, 0);
        erpData->_erpc = getQStringListIndexValue(_linelist, 1);
        erpData->_rhocp = getQStringListIndexValue(_linelist, 2);
        erpData->_erprlf = getQStringListIndexValue(_linelist, 3);
        erpData->_erprefdb = getQStringListIndexValue(_linelist, 4);
        currentline = _file.readLine().simplified();
    }

    void mIMxFile::readPeakFilterData()
    {
        // 全局峰值筛选数据：*peakout, type=X, scale=Y, name="Z"
        QString name = getKeyValue(currentline, "name", "=");
        if (!name.isEmpty())
        {
            // 全局定义行
            name = ExistRenamePhysicalName(Physical_PeakFilter, name);

            int pfID = _modeldata->getGlobalParam()->createPeakFilterID();
            mPeakFilterData* pfData = new mPeakFilterData(pfID, name);

            pfData->_respType = getKeyValue(currentline, "type", "=");
            pfData->_identType = getKeyValue(currentline, "scale", "=");

            currentline = _file.readLine().simplified();
            _linelist = currentline.split(",");
            pfData->_extractCount = getQStringListIndexValue(_linelist, 0);
            pfData->_minSpacing = getQStringListIndexValue(_linelist, 1);
            pfData->_maxSpacing = getQStringListIndexValue(_linelist, 2);
            pfData->_startFreq = getQStringListIndexValue(_linelist, 3);
            pfData->_endFreq = getQStringListIndexValue(_linelist, 4);

            currentline = _file.readLine().simplified();
            while (!_file.atEnd())
            {
                if (currentline.startsWith("*"))
                    break;
                if (currentline.isEmpty())
                {
                    currentline = _file.readLine().simplified();
                    continue;
                }

                _linelist = currentline.split(",");
                QVector<double> row;
                if (_linelist.size() >= 3)
                {
                    row.append(_linelist[0].toDouble());
                    row.append(_linelist[1].toDouble());
                    row.append(_linelist[2].toDouble());
                    pfData->_tableData.append(row);
                }
                currentline = _file.readLine().simplified();
            }

            _modeldata->getGlobalParam()->addPeakFilterData(pfData);
        }
        else
        {
            // 不吞行：仅跳过当前 *peakout 行本身
            currentline = _file.readLine().simplified();
        }
    }
}

