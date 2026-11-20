#include "mMaterialPanel.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDataStatusManager.h"
#include "mOtherFunction.h"
//BasicData
#include "mModelData.h"
#include "mMaterialBase.h"
//#include "mEmptyMaterial.h"
//#include "mIsotropyMaterial.h"

//commond
#include "mCommondSystem.h"
#include "mCommonds.h"

#include <QElapsedTimer>

using namespace MechPreData;
using namespace MechMaterialData;
namespace MechMainWindow
{
    mMaterialPanel::mMaterialPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_pMainLayout = new QVBoxLayout(this);
        m_pMainLayout->setContentsMargins(2, 0, 2, 0);

        m_PanelConfig[LinearElastic_Isotropic] = { MAT_MaterialDefine, MAT_Isotropy,MAT_Density,MAT_MaterialDamping,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[LinearElastic_Orthotropic] = { MAT_MaterialDefine,MAT_Orthotropy,MAT_Density,MAT_MaterialDamping,MAT_CoefficientOfThermalExpansion ,MAT_HeatConductivityCoefficient ,MAT_Heat ,
        MAT_YieldStress ,MAT_YieldStrain ,MAT_FailureHashin ,MAT_EnergyDamage,NONETYPE };
        m_PanelConfig[LinearElastic_Anisotropic] = { MAT_MaterialDefine, MAT_Anisotropy,MAT_Density,MAT_MaterialDamping,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };

        m_PanelConfig[Plastic_Normal] = { MAT_MaterialDefine, MAT_Flex,MAT_Plasticity,MAT_CyclicHardeningWithTable,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,MAT_CreepBehavior_Check,NONETYPE };
        m_PanelConfig[Plastic_Cyclic] = { MAT_MaterialDefine, MAT_Flex,MAT_PlasticityWithOrder,MAT_CyclicHardening,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[Plastic_Damage] = { MAT_MaterialDefine,MAT_Deformation,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[Plastic_Johnson] = { MAT_MaterialDefine, MAT_Flex, MAT_PlasticityJohnson, MAT_RateDependent, MAT_ShearDamage, MAT_Density, MAT_HeatConductivityCoefficient, MAT_Heat, NONETYPE };

        m_PanelConfig[HyperElastic_MooneyRivlin] = { MAT_MaterialDefine, MAT_MooneyRivlin,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_NeoHookean] = { MAT_MaterialDefine, MAT_Neohookean2,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_Ogden] = { MAT_MaterialDefine, MAT_Ogden,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_Yeoh] = { MAT_MaterialDefine, MAT_Yeoh,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_ArrudaBoyce] = { MAT_MaterialDefine, MAT_Arrudaboyce,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[HyperElastic_Polynomial] = { MAT_MaterialDefine, MAT_Polynomial,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };

        m_PanelConfig[Viscous_Viscoelastic] = { MAT_MaterialDefine, MAT_Viscoelasticity,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[Viscous_Viscoplastic] = { MAT_MaterialDefine, MAT_Flex,MAT_CreepBehavior,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };

        m_PanelConfig[Thermal_Base] = { MAT_MaterialDefine,MAT_HeatConductivityCoefficient_UnCheck,MAT_Density,MAT_Heat,MAT_HeatGenerationRate,NONETYPE };

        m_PanelConfig[OtherMaterial_Shim] = { MAT_MaterialDefine, MAT_ThicknessBehavior,MAT_FilmBehavior,MAT_CutBehavior,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[OtherMaterial_Brittle] = { MAT_MaterialDefine, MAT_HardBrittle,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[OtherMaterial_Foam] = { MAT_MaterialDefine, MAT_Hyperhorm,MAT_Density,MAT_CoefficientOfThermalExpansion,MAT_HeatConductivityCoefficient,MAT_Heat,NONETYPE };
        m_PanelConfig[OtherMaterial_Break] = { MAT_MaterialDefine ,MAT_BreakParam, MAT_PhaseFieldParam,NONETYPE };

        m_PanelConfig[AcousticMaterial_Base] = { MAT_MaterialDefine ,MAT_SoundVelocity ,MAT_Density_UnCheck ,MAT_BulkModulus ,MAT_AcousticMaterialDamping,NONETYPE };

        m_PanelConfig[Damage_Shear] = {MAT_MaterialDefine, MAT_ShearDamage, NONETYPE};

        m_PanelConfig[OtherMaterial_LIBRARY] = { MAT_LibCommon ,MAT_LibThermal,MAT_LibStructural };

        m_SelectProps = { MAT_Density ,MAT_MaterialDamping ,MAT_CoefficientOfThermalExpansion ,MAT_HeatConductivityCoefficient ,MAT_Heat ,MAT_YieldStress ,MAT_YieldStrain ,MAT_FailureHashin ,MAT_EnergyDamage ,MAT_CreepBehavior_Check ,
        MAT_HeatGenerationRate ,MAT_BulkModulus ,MAT_AcousticMaterialDamping, MAT_RateDependent, MAT_ShearDamage };
    }

    mMaterialPanel::~mMaterialPanel()
    {
    }

    //在这里加下拉框 创材料 默认创建 线弹性-各向同性MAT_Isotropy
    void mMaterialPanel::createMaterial(MxFunctions::MaterialType matType)
    {
        //新建空材料
        int matID = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>
            (Physical_Material).createID();
        QString matname = "Material";
        int id = 1;
        QString name = matname + "_" + MxFunctions::mNumberToString(id);
        while (getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>
            (Physical_Material).ExistMaterialName(name))
        {
            ++id;
            name = matname + "_" + MxFunctions::mNumberToString(id);
        }

        //默认创建 
        m_MatData = std::make_shared<mMaterialBase>(matID, matType, name);
        getCVM->getModelData()->addPhysicalFieldP(m_MatData);
        setEditing(true);

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建材料 %1").arg(m_MatData->getMaterialName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMaterialSig(true, m_MatData->getMaterialId());

        mCommondMaterial* commond = new mCommondMaterial(mCommondBase::CommondType::MAT_CREATEUNDO);
        commond->storeSnap(m_MatData);
        mCommondSystem::getInstance()->addCommond(commond);
    }

    void mMaterialPanel::editMaterial(int matId)
    {
        //获取材料数据
        m_MatData = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(matId);
        if (m_MatData == nullptr)
        {
            return;
        }
        setEditing(true);

        for (auto w : m_foldWidgets)
        {
            w->setParent(nullptr); // 仅脱离布局，对象保留，不删除
        }
        m_foldWidgets.clear();
        //布局添加输入控件
        QLayoutItem* lastItem = m_pMainLayout->itemAt(m_pMainLayout->count() - 1);
        if (lastItem && lastItem->spacerItem())
        {
            m_pMainLayout->removeItem(lastItem);
        }

        //切换节点：渲染期间抑制收集回写（fold 缓存跨材料复用，渲染中收集会把上一材料的值混写进新材料）
        mMaterialPropBase::s_unitSyncing = true;
        //材料库材料
        MxFunctions::MaterialType tempType = m_MatData->getMaterialType();
        if (tempType == OtherMaterial_LIBRARY)
        {
            QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>> props = m_MatData->getAllLibProps();
            // 按排序后的key遍历
            for (const MxFunctions::MaterialClass& key : m_PanelConfig[tempType])
            {
                auto pPropBase = mMaterialProps::getInstance()->getPropByType(key);
                if (pPropBase)
                {
                    pPropBase->updatePropUnit();
                    pPropBase->setInputsValue(props[key]);
                    pPropBase->bindMaterial(m_MatData);
                    //Panel添加输入控件
                    m_foldWidgets.append(pPropBase);
                    m_pMainLayout->addWidget(pPropBase);
                }
            }
        }
        else
        {
            QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> props = m_MatData->getAllProps();
            QColor color = m_MatData->getMatColor();
            props[MAT_MaterialDefine][MatProp_MaterialType] = QVariant::fromValue<int>(static_cast<int>(tempType));
            props[NONETYPE][MatProp_MaterialColor] = QVariant::fromValue(color);

            if ((tempType == Damage_Shear || tempType == Plastic_Johnson) && props.contains(MAT_DamageEvolution))
            {
                props[MAT_ShearDamage].unite(props[MAT_DamageEvolution]);
                props.remove(MAT_DamageEvolution);
            }

            bool isShow = false;
            for (auto flod : m_PanelConfig[tempType])
            {
                auto pPropBase = mMaterialProps::getInstance()->getPropByType(flod, this);
                if (pPropBase)
                {
                    if (m_SelectProps.contains(flod) && props.contains(flod))
                        pPropBase->setChecked(true);
                    else if (m_SelectProps.contains(flod))
                        pPropBase->setChecked(false);

                    //设置界面数值
                    pPropBase->updatePropUnit();
                    pPropBase->setInputsValue(props[flod]);
                    pPropBase->bindMaterial(m_MatData);

                    //Panel添加输入控件
                    m_foldWidgets.append(pPropBase);
                    m_pMainLayout->addWidget(pPropBase);

                    //特殊处理，部分选择时折叠框隐藏
                    if (flod == MAT_Plasticity)
                    {
                        int idx = props[flod][MatProp_Curve_PlasticityProperties_type].toInt();
                        if (idx == 2)
                        {
                            isShow = true;
                        }
                    }

                    connectSig(pPropBase);
                }
            }

            if (tempType == Plastic_Normal && !isShow)
            {
                int idex = m_foldWidgets.size();
                m_foldWidgets[idex - 7]->hide();
            }
            else if (tempType == Plastic_Cyclic)
            {
                int idex = m_foldWidgets.size();
                m_foldWidgets[idex - 6]->show();
            }

        };
        //单位响应下放到各属性块：编辑态逐个连接，并刷新一次标签防编辑态外切换导致的过期
        for (auto fold : m_foldWidgets)
        {
            if (auto prop = qobject_cast<mMaterialPropBase*>(fold))
            {
                prop->connectUnitSystemResponse();
                prop->updatePropUnit();
            }
        }
        mMaterialPropBase::s_unitSyncing = false;   //渲染结束，恢复收集
        slot_finishEdit();
        m_pMainLayout->addStretch();

        //材料属性块经 setParent(nullptr) 重挂后首次显示时布局未激活，
        //FoldSection 内容区高度被钳在旧 sizeHint 上（密度等折叠框首次展开
        //空白、收起再展开才恢复）。装配完成后统一 refreshLayout：内部延迟
        //5ms 待布局激活再按真实 sizeHint 重算 maximumHeight
        for (auto fold : m_foldWidgets)
        {
            if (fold)
                fold->refreshLayout();
        }
    }

    void mMaterialPanel::addMatProp(const MxFunctions::MaterialClass& prop)
    {
        //addMaterialProp(prop);
        //curProps.insert(prop);
        slot_finishEdit();
    }

    void mMaterialPanel::remMatProp(const MxFunctions::MaterialClass& prop)
    {
        removeMaterialProp(prop);
        //if (curProps.contains(prop))
        //{
        //    curProps.remove(prop);
        //}
        slot_finishEdit();
    }

    void mMaterialPanel::addMaterial(const QString& name, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& props)
    {
        //新建
        auto& manager = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        int matID = manager.createID();
        m_MatData = std::make_shared<mMaterialBase>(matID, MAT_Isotropy);
        getCVM->getModelData()->addPhysicalFieldP(m_MatData);
        int matcnt = 1;
        QString matName = name;
        while (getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).ExistMaterialName(matName))
        {
            matName = name + "_" + MxFunctions::mNumberToString(matcnt++);
        }
        m_MatData->setMaterialName(matName);


        m_MatData->setProperty(props);
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建材料 %1").arg(m_MatData->getMaterialName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMaterialSig(true, m_MatData->getMaterialId());

        mCommondMaterial* commond = new mCommondMaterial(mCommondBase::CommondType::MAT_CREATEUNDO);
        commond->storeSnap(m_MatData);
        mCommondSystem::getInstance()->addCommond(commond);
    }

    void mMaterialPanel::addMaterialFromLib(const QString& name, const QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>>& props)
    {
        auto& manager = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        int matID = manager.createID();
        m_MatData = std::make_shared<mMaterialBase>(matID, OtherMaterial_LIBRARY);
        getCVM->getModelData()->addPhysicalFieldP(m_MatData);
        int matcnt = 1;
        QString matName = name;
        while (getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).ExistMaterialName(matName))
        {
            matName = name + "_" + MxFunctions::mNumberToString(matcnt++);
        }
        m_MatData->setMaterialName(matName);

        m_MatData->setLibProperty(props);
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功插入材料 %1").arg(m_MatData->getMaterialName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMaterialSig(true, m_MatData->getMaterialId());

        mCommondMaterial* commond = new mCommondMaterial(mCommondBase::CommondType::MAT_CREATEUNDO);
        commond->storeSnap(m_MatData);
        mCommondSystem::getInstance()->addCommond(commond);
    }

    void mMaterialPanel::slot_finishEdit()
    {
        if (m_MatData == nullptr)      
            return;
        updatePropsAndFindDiff();

        //更新树节点
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Material, m_MatData->getMaterialId());
    }

    void mMaterialPanel::slot_colorSelectedFinished(QColor color)
    {
        if (m_MatData == nullptr)
        {
            return;
        }
        m_MatData->setMatColor(color);

        getCVM->updatePartShowColorByMat(m_MatData->getMaterialId());

    }

    void mMaterialPanel::slot_hideFold1(int index)
    {
        int idex = m_foldWidgets.size() - 1;
        if (idex < 5)
            return;
        if (index != 0)
        {
            m_foldWidgets[idex - 1]->hide();
            m_foldWidgets[idex - 2]->hide();
            m_foldWidgets[idex - 3]->hide();
            m_foldWidgets[idex - 4]->hide();
            m_foldWidgets[idex - 5]->hide();
        }
        else
        {
            m_foldWidgets[idex - 1]->show();
            m_foldWidgets[idex - 2]->show();
            m_foldWidgets[idex - 3]->show();
            m_foldWidgets[idex - 4]->show();
            m_foldWidgets[idex - 5]->show();
        }
    }

    //切换材料类别
    void mMaterialPanel::slot_TypeChanged(MxFunctions::MaterialType type)
    {
        if (type == m_MatData->getMaterialType())
            return;
        m_MatData->setMaterialType(type);
        editMaterial(m_MatData->getMaterialId());
    }

    void mMaterialPanel::slot_type1_1Changed(int index) //硬化模型
    {
        int idex = m_foldWidgets.size() - 7;
        if (index == 0) //各向同性硬化
        {
            m_foldWidgets[idex]->hide();
        }
        else if (index == 1) //随动硬化
        {
            m_foldWidgets[idex]->hide();
        }
        else if (index == 2) //组合硬化
        {
            m_foldWidgets[idex]->show();
        }
    }

    void mMaterialPanel::connectSig(mMaterialPropBase* prop)
    {
        if (!prop)
            return;
        // 先断开旧连接，防止重复连接导致编辑一个单元格时 slot_finishEdit 被触发多次
        disconnectSig(prop);
        connect(prop, &mMaterialPropBase::sg_editFinished, this, &mMaterialPanel::slot_finishEdit, Qt::UniqueConnection);
        prop->connTable(true);

        if (prop->getPropertyClass() == MAT_Orthotropy)
        {
            connect(prop, &mMaterialPropBase::hideFold, this, &mMaterialPanel::slot_hideFold1, Qt::UniqueConnection);
        }
        if (prop->getPropertyClass() == MAT_Plasticity)
        {
            connect(prop, &mMaterialPropBase::sg_hardModelChanged, this, &mMaterialPanel::slot_type1_1Changed, Qt::UniqueConnection);
        }
        if (prop->getPropertyClass() == MAT_MaterialDefine)
        {
            connect(prop, &mMaterialPropBase::sig_qComBoxTypeChanged, this, &mMaterialPanel::slot_TypeChanged, Qt::UniqueConnection);
        }
        if (prop->getPropertyClass() == NONETYPE)
        {
            connect(qobject_cast<mDefaultProperty*>(prop), &mDefaultProperty::sg_colorSelected, this, &mMaterialPanel::slot_colorSelectedFinished, Qt::UniqueConnection);
        }

        connect(prop, &FoldWidget::enabledChanged, this, [this, prop](bool checked) {
            updateFoldCheckState(prop->getPropertyClass(), checked);
            }, Qt::UniqueConnection);

    }

    void mMaterialPanel::disconnectSig(mMaterialPropBase* prop)
    {
        if (!prop)
            return;
        //disconnect(prop, &mMaterialPropBase::sg_editFinished, this, &mMaterialPanel::slot_finishEdit);
        prop->connTable(false);
        //connect(prop, &mMaterialPropBase::sig_qComBoxTypeChanged, this, &mMaterialPanel::slot_TypeChanged);
        //disconnect(prop, &mMaterialPropBase::sg_hardModelChanged, this, &mMaterialPanel::slot_type1_1Changed);
        //disconnect(prop, &mMaterialPropBase::hideFold, this, &mMaterialPanel::slot_hideFold1);
    }

    void mMaterialPanel::updateFoldCheckState(MxFunctions::MaterialClass matClass, bool encheckable)
    {
        auto matId = m_MatData->getMaterialId();

        //触发重新收集
        slot_finishEdit();
    }

    void mMaterialPanel::updatePropsAndFindDiff()
    {
        if (mMaterialPropBase::s_unitSyncing)   //单位重渲染期间拒绝任何收集回写（双保险，任何触发路径兜底）
            return;
        if (m_MatData == nullptr)
        {
            return;
        }

        QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> propsBefore = m_MatData->getAllProps();
        MxFunctions::MaterialType matTypeBefore = m_MatData->getMaterialType();
        QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> newProps;

        MxFunctions::MaterialType matType = NONEMATERIALTYPE;

        //更新材料类型，属性
        if (matTypeBefore != OtherMaterial_LIBRARY)
        {
            bool addProp = true;
            for (auto flod : m_PanelConfig[matTypeBefore])
            {
                auto prop = mMaterialProps::getInstance()->getPropByType(flod);
                if (prop && prop->isChecked())
                {
                    QHash<MxFunctions::MaterialPropType, QVariant>collectProps = prop->collectProperties();
                    //特殊处理
                    if (flod == MAT_Plasticity)
                    {
                        int idx = collectProps[MatProp_Curve_PlasticityProperties_type].toInt();
                        if (idx != 2)
                        {
                            addProp = false;
                            newProps[flod].unite(collectProps);
                            continue;
                        }
                    }
                    if (addProp)
                        newProps[flod].unite(collectProps);
                    else
                        addProp = true;
                }
            }
            m_MatData->setProperty(newProps);
        }
        else
        {
            QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>> libProps;
            for (auto flod : m_PanelConfig[matTypeBefore])
            {
                auto prop = mMaterialProps::getInstance()->getPropByType(flod);
                if (prop)
                {
                    libProps[flod].unite(prop->collectLibProperties());
                }
            }
            m_MatData->setLibProperty(libProps);
            return;
        }
        if (matTypeBefore != matType)
        {
            mCommondMaterialProp* commond = new mCommondMaterialProp(mCommondBase::CommondType::MAT_PROPMODUNDO);
            commond->storeMatID(m_MatData->getMaterialId());
            //commond->storeProp(propsbefore);
            commond->storeMatType(matTypeBefore);
            mCommondSystem::getInstance()->addCommond(commond);
        }
        else
        {
            //通用属性差异
            int cntbefore = propsBefore.size();
            int cntcurrent = newProps.size();
            if (cntbefore == cntcurrent)
            {
                return;
            }
            int diff = cntcurrent - cntbefore;
            mCommondMaterialProp* commond;
            mCommondBase::CommondType comtype;
            comtype = diff > 0 ? mCommondBase::CommondType::MAT_PROPADDUNDO : mCommondBase::CommondType::MAT_PROPDELUNDO;
            commond = new mCommondMaterialProp(comtype);
            commond->storeMatID(m_MatData->getMaterialId());
            QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> diffprops;
            if (diff > 0)
            {
                for (auto it = newProps.constBegin(); it != newProps.constEnd(); it++)
                {
                    if (!propsBefore.contains(it.key()))
                    {
                        diffprops.insert(it.key(), it.value());
                    }
                }
            }
            else
            {
                for (auto it = propsBefore.constBegin(); it != propsBefore.constEnd(); it++)
                {
                    if (!newProps.contains(it.key()))
                    {
                        diffprops.insert(it.key(), it.value());
                    }
                }
            }
            commond->storeProp(diffprops);
            mCommondSystem::getInstance()->addCommond(commond);
        }
    }


    void mMaterialPanel::removeMaterialProp(const MxFunctions::MaterialClass& propname)
    {
        auto prop = mMaterialProps::getInstance()->getPropByType(propname);
        if (prop == nullptr)
        {
            return;
        }

        for (int i = _propids.size() - 1; i >= 0; --i)
        {
            if (_propids[i].first != propname)
                continue;

            int propid = _propids[i].second;
            FoldSection* foldsection = m_foldWidgets[propid];
            if (foldsection == nullptr)
            {
                _propids.removeAt(i);
                _props.removeAt(i);
                m_fold2PropsVec.remove(i);
                continue;
            }

            // 布局移除控件，但不释放；缓存会保留该 FoldSection 供下次复用
            m_pMainLayout->removeWidget(foldsection);
            foldsection->hide();

            m_foldWidgets[propid] = nullptr;
            m_foldWidgets.removeAt(propid);

            _propids.removeAt(i);
            _props.removeAt(i);
            m_fold2PropsVec.remove(i);

            --propcnt;

            disconnectSig(prop);
        }

        //removeDefaultProp();

        propcnt = 1;
        int lCout = m_pMainLayout->count();
        int foldCount = m_foldWidgets.size();
        int foldCount1 = m_foldWidgets.size();
    }

    void mMaterialPanel::clearAllPropFolds()
    {
        // 仅清理当前显示在 m_foldWidgets / _propids 中的属性折叠块，不删除缓存中的 FoldSection
        for (int i = _propids.size() - 1; i >= 0; --i)
        {
            int propid = _propids[i].second;
            FoldSection* foldsection = m_foldWidgets[propid];
            if (foldsection)
            {
                m_pMainLayout->removeWidget(foldsection);
                foldsection->hide();
            }
            m_foldWidgets[propid] = nullptr;
        }
        m_foldWidgets.removeAll(nullptr);

        _propids.clear();
        _props.clear();
        m_fold2PropsVec.clear();
        propcnt = 1;
    }

    void mMaterialPanel::unEditMaterial()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        m_MatData = nullptr;
        //退出编辑态：逐个断开各属性块的单位响应
        for (auto fold : m_foldWidgets)
            if (auto prop = qobject_cast<mMaterialPropBase*>(fold))
            {
                prop->disconnectUnitSystemResponse();
                prop->bindMaterial(nullptr);
            }
        // 隐藏所有属性折叠块，保留缓存避免重复创建
        //clearAllPropFolds();

        // 移除默认属性折叠块
        //removeDefaultProp();
    }

    void mMaterialPanel::clearPropData(const MxFunctions::MaterialClass& propname)
    {
        auto prop = mMaterialProps::getInstance()->getPropByType(propname);
        if (prop == nullptr || m_MatData == nullptr)
        {
            return;
        }
        prop->clearData();
    }

    void mMaterialPanel::updatePanelUnit()
    {
        mMaterialProps::getInstance()->updateAllPropUnit();
    }
}