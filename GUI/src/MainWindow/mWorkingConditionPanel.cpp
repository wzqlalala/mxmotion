#include "mWorkingConditionPanel.h"
#include "mWorkingConditions.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mLoadGravityPanel.h"
#include "mBoundaryFixedPanel.h"
#include "mBoundaryTempPanel.h"

//commond
#include "mCommondSystem.h"
#include "mCommonds.h"

//Basic
#include<mMixSet.h>
#include<mFunctionBase.h>
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mLoadBoundaryBase.h"
#include "mLoadBoundaryManager.h"
#include "mLoadBolt.h"
#include "mLoadAcousticConcForce.h"
#include "mLoadAcousticFrequencyDomain.h"
#include "mBoundarySuperelement.h"

#include "mPredefinedFieldInitTemp.h"
#include "mPredefinedFieldManager.h"
#include "mRotateAnalyseManager.h"
#include "mDataStatusManager.h"
#include "mLoadRotationalACC.h"
#include "mLoadRotationalSpeed.h"

#include <QLabel>

#include "AllRenderSymbolManager.h"
#include "mOtherFunction.h"
#include "mLoadEdge.h"

using namespace MechPreData;
using namespace MechFunctionData;


namespace MechMainWindow
{
	mWorkingConditionPanel::mWorkingConditionPanel(QWidget* parent)
		: mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->addStretch();
	}

	mWorkingConditionPanel::~mWorkingConditionPanel()
	{}

    void mWorkingConditionPanel::createLoad(MxFunctions::LoadBoundaryType type)
    {
        //空白处右键创建时不经过树选中变化，旧面板的折叠框不会被卸载，需先收尾清理
        if (isEditing())
        {
            switch (_editingType)
            {
            case EditingType::Load: unEditLoad(); break;
            case EditingType::Boundary: unEditBoundary(); break;
            case EditingType::PredefinedField: unEditPredefinedField(); break;
            case EditingType::RotateAnalyse: unEditRotateAnalyse(); break;
            default: break;
            }
        }
        //数据
        mLoadBoundaryManager& manager = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
        _loadBoundaryData = nullptr;

        switch (type)
        {
        case LOAD_IM_FORCE:
        {
            auto newload = std::make_shared<MechCaseData::mLoadForce>(manager.createID(), LOAD_IM_FORCE, getNewLoadName("Force"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_TORQUE:
        {
            auto newload = std::make_shared<MechCaseData::mLoadForce>(manager.createID(), LOAD_IM_TORQUE, getNewLoadName("Moment"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_PRESSURE:
        {
            auto newload = std::make_shared<MechCaseData::mLoadPressure>(manager.createID(), LOAD_IM_PRESSURE, getNewLoadName("Pressure"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_GRAVITY:
        {
            auto newload = std::make_shared<MechCaseData::mLoadGravity>(manager.createID(), LOAD_IM_GRAVITY, getNewLoadName("Gravity"));
            _loadBoundaryData = newload;
        }
        break;
        //热学载荷
        case LOAD_IM_HEATFLUX:
        {
            auto newload = std::make_shared<MechCaseData::mLoadHeatFlux>(manager.createID(), LOAD_IM_HEATFLUX, getNewLoadName("HeatFlux"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_RADIATION:
        {
            auto newload = std::make_shared<MechCaseData::mLoadRadiation>(manager.createID(), LOAD_IM_RADIATION, getNewLoadName("Radiation"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_CONVECTION:
        {
            auto newload = std::make_shared<MechCaseData::mLoadConvection>(manager.createID(), LOAD_IM_CONVECTION, getNewLoadName("Convection"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_TRANSIENT: //时域载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadTransient>(manager.createID(), LOAD_IM_TRANSIENT, getNewLoadName("Transient"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_FREQDOMAIN: //频域载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadFreqdomain>(manager.createID(), LOAD_IM_FREQDOMAIN, getNewLoadName("FreqLoad"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_COMBINED: //组合载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadCombined>(manager.createID(), LOAD_IM_COMBINED, getNewLoadName("LoadCombo"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_BOLT: //螺栓载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadBolt>(manager.createID(), LOAD_IM_BOLT, getNewLoadName("BoltLoad"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_ACOCONCFORCE: //声学载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadAcousticConcForce>(manager.createID(), LOAD_IM_ACOCONCFORCE, getNewLoadName("AcousticForce"));
            _loadBoundaryData = newload;
        } 
        break;
        case LOAD_IM_ACOFREQDOMAIN: //声学频域载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadAcousticFrequencyDomain>(manager.createID(), LOAD_IM_ACOFREQDOMAIN, getNewLoadName("AcousticFreq"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_SPECTRUM: //谱载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadSpectrum>(manager.createID(), LOAD_IM_SPECTRUM, getNewLoadName("Spectrum"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_ROTATE_PHYSICAL_POWER: //旋转体力
        {
            auto newload = std::make_shared<MechCaseData::mRotatePhysicalPower>(manager.createID(), LOAD_ROTATE_PHYSICAL_POWER, getNewLoadName("RotatePhysicalPower"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_SURFACE: //表面载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadSurface>(manager.createID(), LOAD_IM_SURFACE, getNewLoadName("SurfaceLoad"));
            _loadBoundaryData = newload;
        }
        break;
        case LOAD_IM_EDGE: //边载荷
        {
            auto newload = std::make_shared<MechCaseData::mLoadEdge>(manager.createID(), LOAD_IM_EDGE, getNewLoadName("EdgeLoad"));
            _loadBoundaryData = newload;
        }
        break;
		default:
			return;
		}

		getCVM->getModelData()->addPhysicalFieldP(_loadBoundaryData);
		setEditing(true);
		_editingType = EditingType::Load;

		//创建完成
		mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建载荷 %1").arg(_loadBoundaryData->getLoadBoundaryName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateLoadSig(true, _loadBoundaryData->getLoadBoundaryId());


        mCommondLoad* commond = new mCommondLoad(mCommondBase::CommondType::LOAD_CREATEUNDO);
        commond->_loadData = _loadBoundaryData;
        mCommondSystem::getInstance()->addCommond(commond);
    }

    void mWorkingConditionPanel::createBoundary(MxFunctions::LoadBoundaryType type)
    {
        //空白处右键创建时不经过树选中变化，旧面板的折叠框不会被卸载，需先收尾清理
        if (isEditing())
        {
            switch (_editingType)
            {
            case EditingType::Load: unEditLoad(); break;
            case EditingType::Boundary: unEditBoundary(); break;
            case EditingType::PredefinedField: unEditPredefinedField(); break;
            case EditingType::RotateAnalyse: unEditRotateAnalyse(); break;
            default: break;
            }
        }
        //数据
        mLoadBoundaryManager& manager = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
        _loadBoundaryData = nullptr;

        switch (type)
        {
        case BOUNDARY_IM_FIXED:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundaryFixed>(manager.createID(), BOUNDARY_IM_FIXED, getNewBoundaryName("Encastre"));
            _loadBoundaryData = newboundary;
        }
        break;
        case BOUNDARY_IM_DISPLACEMENT:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundaryDisplacement>(manager.createID(), type, getNewBoundaryName("DispRot"));
            _loadBoundaryData = newboundary;
        }
        break;
        case BOUNDARY_IM_SPEED:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundaryDisplacement>(manager.createID(), type, getNewBoundaryName("Speed"));
            _loadBoundaryData = newboundary;
        }
        break;
        case BOUNDARY_IM_ACC:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundaryDisplacement>(manager.createID(), type, getNewBoundaryName("Acceleration"));
            _loadBoundaryData = newboundary;
        }
        break;
        case BOUNDARY_IM_TEMP:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundaryTemp>(manager.createID(), BOUNDARY_IM_TEMP, getNewBoundaryName("Temperature"));
            _loadBoundaryData = newboundary;
        }
        break;
        case BOUNDARY_IM_VIRTUAL:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundaryVirtual>(manager.createID(), BOUNDARY_IM_VIRTUAL, getNewBoundaryName("VirtualCons"));
            _loadBoundaryData = newboundary;
        }
        break;
        case BOUNDARY_IM_SUPERELEMENT:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundarySuperelement>(manager.createID(), BOUNDARY_IM_SUPERELEMENT, getNewBoundaryName("GeneralCons"));
            _loadBoundaryData = newboundary;
        }
        break;
        case BOUNDARY_MOVE:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundaryMove>(manager.createID(), BOUNDARY_MOVE, getNewBoundaryName("Move"));
            _loadBoundaryData = newboundary;
        }
        break;
        case BOUNDARY_TEMP_NEW:
        {
            auto newboundary = std::make_shared<MechCaseData::mBoundaryTempNew>(manager.createID(), BOUNDARY_TEMP_NEW, getNewBoundaryName("TemperatureNew"));
            _loadBoundaryData = newboundary;
        }
        break;
		default:
			return;
		}

		getCVM->getModelData()->addPhysicalFieldP(_loadBoundaryData);
		setEditing(true);
		_editingType = EditingType::Boundary;

		//创建完成
		mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建边界 %1").arg(_loadBoundaryData->getLoadBoundaryName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateBoundarySig(true, _loadBoundaryData->getLoadBoundaryId());

        mCommondBoundary* commond = new mCommondBoundary(mCommondBase::CommondType::BOUNDARY_CREATEUNDO);
        commond->_boundaryData = _loadBoundaryData;
        mCommondSystem::getInstance()->addCommond(commond);
    }

    void mWorkingConditionPanel::createPredefinedField(MxFunctions::PredefinedFieldType type)
    {
        //空白处右键创建时不经过树选中变化，旧面板的折叠框不会被卸载，需先收尾清理
        if (isEditing())
        {
            switch (_editingType)
            {
            case EditingType::Load: unEditLoad(); break;
            case EditingType::Boundary: unEditBoundary(); break;
            case EditingType::PredefinedField: unEditPredefinedField(); break;
            case EditingType::RotateAnalyse: unEditRotateAnalyse(); break;
            default: break;
            }
        }
        //数据
        mPredefinedFieldManager& manager = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField);
        _predefinedFieldData = nullptr;

        switch (type)
        {
        case PREDEFINED_FIELD_INIT_TEMP:
        {
            auto newfield = std::make_shared<MechCaseData::mPredefinedFieldInitTemp>(manager.createID(), PREDEFINED_FIELD_INIT_TEMP, getNewPredefinedFieldName("InitialTemp"));
            _predefinedFieldData = newfield;
        }
        break;
        case PREDEFINED_FIELD_TEMP_LOAD:
        {
            auto newfield = std::make_shared<MechCaseData::mPredefinedFieldTempLoad>(manager.createID(), PREDEFINED_FIELD_TEMP_LOAD, getNewPredefinedFieldName("TempLoad"));
            _predefinedFieldData = newfield;
        }
        break;
		default:
			return;
		}

		getCVM->getModelData()->addPhysicalFieldP(_predefinedFieldData);
		setEditing(true);
		_editingType = EditingType::PredefinedField;

		//创建完成
		mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建预定义场 %1").arg(_predefinedFieldData->getPredefinedFieldName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePredefinedFieldSig(true, _predefinedFieldData->getPredefinedFieldId());
    }

    void mWorkingConditionPanel::loadPanel(MxFunctions::LoadBoundaryType type, int caseId)
    {
        auto load = mWorkingConditions::getInstance()->getLoadByType(type);
        if (load == nullptr)
        {
            return;
        }

        //获取中文名称
        QString typeName = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryTypeNameByType(type);

        // 载荷定义折叠框
        FoldSection* propFold0 = new FoldSection("载荷定义", this);
        propFold0->addRow(new PanelLabel("载荷类型："), new PanelLabel(typeName));
        m_foldWidgets.append(propFold0);

        FoldSection* propFold3 = new FoldSection("载荷引用状态", this);
        FoldSection* propFold1 = new FoldSection("作用对象", this);
        FoldSection* propFold2 = new FoldSection(typeName + "定义", this);

        QVector<QWidget*>wds3 = load->getRefStateInputs(propFold3);
        QVector<QWidget*>wds1 = load->getTargetInputs(propFold1);
        QVector<QWidget*>wds2 = load->getValueInputs(propFold2);
        for (int i = 0; i < wds3.size(); ++i)
        {
            propFold3->addRow(wds3[i]);
            wds3[i]->show();
        }
        for (int i = 0; i < wds1.size(); ++i)
        {
            propFold1->addRow(wds1[i]);
            wds1[i]->show();
        }
        for (int i = 0; i < wds2.size(); ++i)
        {
            propFold2->addRow(wds2[i]);
            wds2[i]->show();
        }
        m_foldWidgets.append(propFold3);
        m_foldWidgets.append(propFold1);
        m_foldWidgets.append(propFold2);

        // 重力载荷局部施加：作用对象由独立折叠区承载，通过可勾选标题控制启用
        FoldSection* gravityCornerFold = nullptr;
        if (type == LOAD_IM_GRAVITY)
        {
            gravityCornerFold = new FoldSection("局部施加", this);
            gravityCornerFold->setCheckable(true);

            std::shared_ptr<MechCaseData::mLoadGravity> graPtr = std::dynamic_pointer_cast<MechCaseData::mLoadGravity>(_loadBoundaryData);
            // 绑定折叠框勾选状态到数据类，并同步控制通用拾取控件可用性
            if (graPtr)
            {
                gravityCornerFold->setChecked(graPtr->isCornerAddChecked);
                gravityCornerFold->onCheckBoxToggled(graPtr->isCornerAddChecked);
                connect(gravityCornerFold, &FoldWidget::enabledChanged, this, [=](bool checked) {
                    if (graPtr) {
                        graPtr->isCornerAddChecked = checked;
                    }
                    auto gravityPanel = dynamic_cast<mLoadGravityPanel*>(load);
                    if (gravityPanel) {
                        gravityPanel->setPickEnabled(checked);
                    }
                    });
            }

            QVector<QWidget*> wdsCornerAdds = load->getTargetInputs(gravityCornerFold);
            for (int i = 0; i < wdsCornerAdds.size(); ++i)
            {
                gravityCornerFold->addRow(wdsCornerAdds[i]);
                wdsCornerAdds[i]->show();
            }
            // 折叠框在上方 onCheckBoxToggled 时已收起，收起期间 addRow 的行
            // 不会同步进内容区高度：首次展开按收起前的旧 sizeHint 设高度，拾
            // 取控件行被压进旧高度里显示不全，需折叠再展开才恢复。加行后刷新
            // 一次折叠高度缓存（refreshLayout 按当前收起态收尾，不会展开它）
            gravityCornerFold->refreshLayout();
            m_foldWidgets.append(gravityCornerFold);
        }

        QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
        if (lastItem && lastItem->spacerItem())
        {
            _mainLayout->removeItem(lastItem);

            _mainLayout->addWidget(propFold0);

            // 热学载荷分析场方式：无作用对象与大小，隐藏载荷引用状态与作用对象折叠框
            bool isThermalAnalysisFieldMode = false;
            if (type == LOAD_IM_HEATFLUX)
            {
                auto heatFluxPtr = std::dynamic_pointer_cast<MechCaseData::mLoadHeatFlux>(_loadBoundaryData);
                isThermalAnalysisFieldMode = heatFluxPtr && heatFluxPtr->defineType_ == 2;
            }
            else if (type == LOAD_IM_RADIATION)
            {
                auto radiationPtr = std::dynamic_pointer_cast<MechCaseData::mLoadRadiation>(_loadBoundaryData);
                isThermalAnalysisFieldMode = radiationPtr && radiationPtr->defineType_ == 2;
            }
            else if (type == LOAD_IM_CONVECTION)
            {
                auto convectionPtr = std::dynamic_pointer_cast<MechCaseData::mLoadConvection>(_loadBoundaryData);
                // 对流换热分析场方式有作用对象（mx文件有set关键字），需要显示作用对象折叠框
                isThermalAnalysisFieldMode = convectionPtr && convectionPtr->defineType_ == 3;
            }
            else if (type == LOAD_IM_FORCE)
            {
                auto forcePtr = std::dynamic_pointer_cast<MechCaseData::mLoadForce>(_loadBoundaryData);
                isThermalAnalysisFieldMode = forcePtr && forcePtr->isAnalysisFieldMode_;
            }
            else if (type == LOAD_IM_PRESSURE)
            {
                auto pressurePtr = std::dynamic_pointer_cast<MechCaseData::mLoadPressure>(_loadBoundaryData);
                isThermalAnalysisFieldMode = pressurePtr && pressurePtr->pressureType_ == 0 && pressurePtr->isAnalysisFieldMode_;
            }

            //点击载荷管理器不需要缩放因子折叠框 瞬态 频域 组合 声学集中 声学频域无缩放因子
            if (!isThermalAnalysisFieldMode && caseId > 0 && type != LOAD_IM_TRANSIENT && type != LOAD_IM_FREQDOMAIN && type != LOAD_IM_COMBINED
                && type != LOAD_IM_ACOCONCFORCE && type != LOAD_IM_ACOFREQDOMAIN && type != LOAD_IM_SPECTRUM
                && type != LOAD_ROTATE_PHYSICAL_POWER
                && type != BOUNDARY_MOVE && type != BOUNDARY_TEMP_NEW
                )
            {
                _mainLayout->addWidget(propFold3);
            }

            // 集中力/力矩在文件模式或独立赋值模式下不显示作用对象折叠框
            // 独立赋值模式由离散数据控件内部的拾取按钮驱动拾取，无需外置通用拾取控件
            bool isFileMode = false;
            if (type == LOAD_IM_FORCE || type == LOAD_IM_TORQUE)
            {
                std::shared_ptr<MechCaseData::mLoadForce> forcePtr = std::dynamic_pointer_cast<MechCaseData::mLoadForce>(_loadBoundaryData);
                if (forcePtr)
                {
                    bool isForce = (forcePtr->getLoadBoundaryType() == LOAD_IM_FORCE);
                    // 集中力：独立赋值=2、文件=3；力矩：独立赋值=1、文件=2；分析场同样不显示作用对象折叠框
                    const int independentWay = isForce ? 2 : 1;
                    const int fileWay = isForce ? 3 : 2;
                    isFileMode = (forcePtr->way_ == fileWay) || (forcePtr->way_ == independentWay) || forcePtr->isAnalysisFieldMode_;
                }
            }
            else if (type == LOAD_IM_PRESSURE)
            {
                std::shared_ptr<MechCaseData::mLoadPressure> pressurePtr = std::dynamic_pointer_cast<MechCaseData::mLoadPressure>(_loadBoundaryData);
                if (pressurePtr)
                {
                    // 压力：独立赋值=1、文件=2（仅常规压力），两种模式均不显示作用对象折叠框；分析场无作用对象同样不显示
                    isFileMode = (pressurePtr->pressureType_ == 0 && (pressurePtr->way_ == 1 || pressurePtr->way_ == 2 || pressurePtr->isAnalysisFieldMode_));
                }
            }
            else if (type == LOAD_ROTATE_PHYSICAL_POWER)
            {
                // 旋转体力：独立赋值=1，拾取由离散数据控件内部驱动，不显示作用对象折叠框
                std::shared_ptr<MechCaseData::mRotatePhysicalPower> rotatePtr = std::dynamic_pointer_cast<MechCaseData::mRotatePhysicalPower>(_loadBoundaryData);
                if (rotatePtr)
                {
                    isFileMode = (rotatePtr->_defineType == 1);
                }
            }
            else if (type == LOAD_IM_SURFACE)
            {
                // 表面载荷：独立赋值=1，拾取由离散数据控件内部驱动，不显示作用对象折叠框
                std::shared_ptr<MechCaseData::mLoadSurface> surfacePtr = std::dynamic_pointer_cast<MechCaseData::mLoadSurface>(_loadBoundaryData);
                if (surfacePtr)
                {
                    isFileMode = (surfacePtr->way_ == 1);
                }
            }
            else if (type == LOAD_IM_HEATFLUX)
            {
                // 热流密度：独立赋值（defineType_=1）时拾取由离散数据控件内部驱动，不显示作用对象折叠框
                // 分析场（defineType_=2）无作用对象，同样不显示
                std::shared_ptr<MechCaseData::mLoadHeatFlux> heatFluxPtr = std::dynamic_pointer_cast<MechCaseData::mLoadHeatFlux>(_loadBoundaryData);
                if (heatFluxPtr)
                {
                    isFileMode = (heatFluxPtr->defineType_ == 1 || heatFluxPtr->defineType_ == 2);
                }
            }
            else if (type == LOAD_IM_RADIATION)
            {
                // 热辐射：独立赋值（defineType_=1）时拾取由离散数据控件内部驱动，不显示作用对象折叠框
                // 分析场（defineType_=2）无作用对象，同样不显示
                std::shared_ptr<MechCaseData::mLoadRadiation> radiationPtr = std::dynamic_pointer_cast<MechCaseData::mLoadRadiation>(_loadBoundaryData);
                if (radiationPtr)
                {
                    isFileMode = (radiationPtr->defineType_ == 1 || radiationPtr->defineType_ == 2);
                }
            }
            else if (type == LOAD_IM_CONVECTION)
            {
                // 对流换热：独立赋值（defineType_=1）时拾取由离散数据控件内部驱动，不显示作用对象折叠框
                // 分析场（defineType_=3）有作用对象（mx文件有set关键字），需要显示作用对象折叠框
                std::shared_ptr<MechCaseData::mLoadConvection> convectionPtr = std::dynamic_pointer_cast<MechCaseData::mLoadConvection>(_loadBoundaryData);
                if (convectionPtr)
                {
                    isFileMode = (convectionPtr->defineType_ == 1);
                }
            }

            if (type <= 3 && type != LOAD_IM_GRAVITY && !isFileMode)
            {
                _mainLayout->addWidget(propFold1);
            }

            if (type == LOAD_IM_HEATFLUX && !isFileMode || (type == LOAD_IM_CONVECTION && !isFileMode) || (type == LOAD_IM_RADIATION && !isFileMode) || type == LOAD_IM_BOLT
                || type == LOAD_IM_ACOCONCFORCE || (type == LOAD_ROTATE_PHYSICAL_POWER && !isFileMode) || (type == LOAD_IM_CONVECTION && !isFileMode) || (type == LOAD_IM_SURFACE && !isFileMode)
                || type == LOAD_IM_EDGE)
            {
                _mainLayout->addWidget(propFold1);
            }
            _mainLayout->addWidget(propFold2);

            // 重力载荷：局部施加折叠区承载作用对象
            if (type == LOAD_IM_GRAVITY && gravityCornerFold)
            {
                _mainLayout->addWidget(gravityCornerFold);
            }
            _mainLayout->addStretch();
        }
    }

    void mWorkingConditionPanel::boundaryPanel(MxFunctions::LoadBoundaryType type, int caseId)
    {
        auto boundary = mWorkingConditions::getInstance()->getBoundaryByType(type);
        if (boundary == nullptr)
        {
            return;
        }

        //获取中文名称
        QString typeName = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryTypeNameByType(type);

        // 边界定义折叠框
        FoldSection* propFold0 = new FoldSection("边界定义", this);
        propFold0->addRow(new PanelLabel("边界类型："), new PanelLabel(typeName));
        m_foldWidgets.append(propFold0);

        FoldSection* propFold3 = new FoldSection("边界引用状态", this);
        FoldSection* propFold1 = new FoldSection("作用对象", this);
        FoldSection* propFold2 ;

        if (type == BOUNDARY_MOVE || type == BOUNDARY_TEMP_NEW)
        {
            propFold2 = new FoldSection(typeName + "主模型定义", this);
        }
        else
        {
            propFold2 = new FoldSection(typeName + "定义", this);
        }

        QVector<QWidget*>wds3 = boundary->getRefStateInputs(propFold3);
        QVector<QWidget*>wds1 = boundary->getTargetInputs(propFold1);
        QVector<QWidget*>wds2 = boundary->getValueInputs(propFold2);

        // 位移转角/速度/加速度边界：独立赋值模式（_definitionMode==1）下拾取由
        // 离散数据控件内部驱动，不显示作用对象折叠框（与集中力/力矩同一策略）
        // 温度边界：分析场方式无作用对象，同样不显示作用对象与引用状态折叠框
        bool isIndependentMode = false;
        bool isBoundaryAnalysisFieldMode = false;
        if (type == BOUNDARY_IM_DISPLACEMENT || type == BOUNDARY_IM_SPEED || type == BOUNDARY_IM_ACC)
        {
            std::shared_ptr<MechCaseData::mBoundaryDisplacement> dispPtr =
                std::dynamic_pointer_cast<MechCaseData::mBoundaryDisplacement>(_loadBoundaryData);
            if (dispPtr)
            {
                isIndependentMode = (dispPtr->_definitionMode == 1);
            }
        }
        else if (type == BOUNDARY_IM_TEMP)
        {
            std::shared_ptr<MechCaseData::mBoundaryTemp> tempPtr =
                std::dynamic_pointer_cast<MechCaseData::mBoundaryTemp>(_loadBoundaryData);
            if (tempPtr)
            {
                isBoundaryAnalysisFieldMode = tempPtr->isAnalysisFieldMode_;
            }
        }

        for (int i = 0; i < wds3.size(); ++i)
        {
            propFold3->addRow(wds3[i]);
            wds3[i]->show();
        }
        for (int i = 0; i < wds1.size(); ++i)
        {
            propFold1->addRow(wds1[i]);
            wds1[i]->show();
        }
        for (int i = 0; i < wds2.size(); ++i)
        {
            propFold2->addRow(wds2[i]);
            wds2[i]->show();
        }
        m_foldWidgets.append(propFold3);
        m_foldWidgets.append(propFold1);
        m_foldWidgets.append(propFold2);
        QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
        if (lastItem && lastItem->spacerItem())
        {
            _mainLayout->removeItem(lastItem);

            _mainLayout->addWidget(propFold0);

            //点击边界管理器不需要缩放因子折叠框(只有位移/转角 温度边界 有缩放因子)
            //温度边界分析场方式无大小概念，不显示引用状态折叠框
            if (!isBoundaryAnalysisFieldMode && caseId > 0 && type != BOUNDARY_IM_FIXED && type != BOUNDARY_IM_FORWARD && type != BOUNDARY_IM_FORWARD && type != BOUNDARY_IM_VIRTUAL && type != BOUNDARY_IM_SUPERELEMENT
                && type != BOUNDARY_MOVE && type != BOUNDARY_TEMP_NEW)
            {
                _mainLayout->addWidget(propFold3);
            }
            if (!isIndependentMode && !isBoundaryAnalysisFieldMode)
            {
                _mainLayout->addWidget(propFold1);
            }
            else
            {
                // 独立赋值模式不显示作用对象折叠框：仅填充内容但不入布局，
                // 切出时由 unEditBoundary 统一删除，此处保持其隐藏状态
                propFold1->hide();
            }
            _mainLayout->addWidget(propFold2);
            _mainLayout->addStretch();
        }
    }

    void mWorkingConditionPanel::predefinedFieldPanel(MxFunctions::PredefinedFieldType type, int caseId)
    {
        auto field = mWorkingConditions::getInstance()->getPredefinedFieldByType(type);
        if (field == nullptr)
        {
            return;
        }

        // 获取当前定义类型（初始温度/温度载荷）
        std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldPtr = _predefinedFieldData;
        int defineType = 0;
        if (fieldPtr)
        {
            if (type == PREDEFINED_FIELD_INIT_TEMP)
            {
                auto initTempPtr = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldInitTemp>(fieldPtr);
                if (initTempPtr)
                {
                    defineType = initTempPtr->_defineType;
                }
            }
            else if (type == PREDEFINED_FIELD_TEMP_LOAD)
            {
                auto tempLoadPtr = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldTempLoad>(fieldPtr);
                if (tempLoadPtr)
                {
                    defineType = tempLoadPtr->_defineType;
                }
            }
        }

        if (type == PREDEFINED_FIELD_TEMP_LOAD) //温度载荷
        {
            auto tempField = dynamic_cast<mPredefinedFieldTempLoadPanel*>(field);

            // 获取当前定义类型
            std::shared_ptr<MechCaseData::mPredefinedFieldTempLoad> fieldPtr;
            if (_predefinedFieldData)
            {
                fieldPtr = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldTempLoad>(_predefinedFieldData);
            }
            int defineType = 0;
            if (fieldPtr)
            {
                defineType = fieldPtr->_defineType;
            }

            // 获取中文名称
            mPredefinedFieldManager& manager = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField);
            QString typeName = manager.getPredefinedFieldTypeNameByID(_predefinedFieldData->getPredefinedFieldId());

            // 预定义场定义折叠框
            FoldSection* propFold0 = new FoldSection("预定义场定义", this);
            propFold0->addRow(new PanelLabel("预定义场类型："), new PanelLabel(typeName));
            m_foldWidgets.append(propFold0);

            // 预定义场引用状态
            FoldSection* propFold3 = new FoldSection("预定义场引用状态", this);
            QVector<QWidget*>wds3 = field->getRefStateInputs(propFold3);
            for (int i = 0; i < wds3.size(); ++i)
            {
                propFold3->addRow(wds3[i]);
                wds3[i]->show();
            }
            m_foldWidgets.append(propFold3);

            // 统一赋值显示作用对象；独立赋值拾取由离散数据控件直驱、文件定义无拾取，均不显示
            // 统一赋值显示作用对象；独立赋值拾取由离散数据控件直驱、文件定义无拾取。
            // 注意：非统一赋值下仍创建折叠框并入布局（隐藏态），让通用拾取控件始终受布局管理，
            // 避免其无父布局成为浮窗闪现（与边界面板 isIndependentMode 同一策略）。
            FoldSection* propFold1 = new FoldSection("作用对象", this);
            QVector<QWidget*>wds1 = field->getTargetInputs(propFold1);
            for (int i = 0; i < wds1.size(); ++i)
            {
                propFold1->addRow(wds1[i]);
                wds1[i]->show();
            }
            m_foldWidgets.append(propFold1);

            // 温度载荷定义（包含定义方式下拉框）
            FoldSection* propFold2 = new FoldSection("温度载荷定义", this);
            QVector<QWidget*>wds2 = field->getValueInputs(propFold2);
            for (int i = 0; i < wds2.size(); ++i)
            {
                propFold2->addRow(wds2[i]);
                wds2[i]->show();
            }
            m_foldWidgets.append(propFold2);

            // 移除原有spacerItem，避免顶部空白
            QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
            if (lastItem && lastItem->spacerItem())
            {
                _mainLayout->removeItem(lastItem);
            }

            // 按顺序添加折叠块
            _mainLayout->addWidget(propFold0);
            if (caseId > 0)
            {
                _mainLayout->addWidget(propFold3);
            }
            // 非统一赋值：作用对象折叠框入布局但保持隐藏，内部拾取控件仍受布局管理
            propFold1->setVisible(defineType == 0);
            _mainLayout->addWidget(propFold1);
            _mainLayout->addWidget(propFold2);
            _mainLayout->addStretch();
        }
        else
        {
            // 获取中文名称
            mPredefinedFieldManager& manager = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField);
            QString typeName = manager.getPredefinedFieldTypeNameByID(_predefinedFieldData->getPredefinedFieldId());

            // 预定义场定义折叠框
            FoldSection* propFold0 = new FoldSection("预定义场定义", this);
            propFold0->addRow(new PanelLabel("预定义场类型："), new PanelLabel(typeName));
            m_foldWidgets.append(propFold0);

            // 统一赋值显示作用对象；独立赋值拾取由离散数据控件直驱，不显示作用对象折叠框。
            // 注意：隐藏模式下仍创建折叠框并入布局（setPickWidgetVisible(false) 隐藏其内容），
            // 避免通用拾取控件无父布局成为浮窗闪现（与边界面板 isIndependentMode 同一策略）。
            FoldSection* propFold1 = nullptr;
            QVector<QWidget*>wds1;
            {
                propFold1 = new FoldSection("作用对象", this);
                wds1 = field->getTargetInputs(propFold1);
                for (int i = 0; i < wds1.size(); ++i)
                {
                    propFold1->addRow(wds1[i]);
                    wds1[i]->show();
                }
                m_foldWidgets.append(propFold1);
            }
            FoldSection* propFold2 = new FoldSection("初始温度定义", this);

            QVector<QWidget*>wds2 = field->getValueInputs(propFold2);

            for (int i = 0; i < wds2.size(); ++i)
            {
                propFold2->addRow(wds2[i]);
                wds2[i]->show();
            }
            m_foldWidgets.append(propFold2);

            // 移除原有spacerItem
            QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
            if (lastItem && lastItem->spacerItem())
            {
                _mainLayout->removeItem(lastItem);
                _mainLayout->addWidget(propFold0);
                // 独立赋值：作用对象折叠框入布局但保持隐藏，内部拾取控件仍受布局管理
                propFold1->setVisible(defineType == 0);
                _mainLayout->addWidget(propFold1);
                _mainLayout->addWidget(propFold2);
                _mainLayout->addStretch();
            }
        }
    }

	void mWorkingConditionPanel::rotateAnalysePanel(MxFunctions::RotateAnalysyType type, int caseId)
	{
		auto rotate = mWorkingConditions::getInstance()->getRotateAnalyseByType(type);
		if (rotate == nullptr)
		{
			return;
		}

		mRotateAnalyseManager& manager = getCVM->getModelData()->getManager<mRotateAnalyseManager>(Physical_RotateAnalyse);
		QString typeName = manager.getRotateAnalyseTypeNameByID(_rotateAnalyseData->getRotateAnalyseId());

		FoldSection* propFold0 = new FoldSection("旋转分析定义", this);
		propFold0->addRow(new PanelLabel("旋转分析类型："), new PanelLabel(typeName));
		m_foldWidgets.append(propFold0);

		FoldSection* propFold1 = nullptr;
		FoldSection* propFold2 = new FoldSection("旋转分析定义", this);

		QVector<QWidget*>wds1 = rotate->getTargetInputs(nullptr);
		QVector<QWidget*>wds2 = rotate->getValueInputs(propFold2);

		// 仅当旋转分析面板确实需要作用对象控件时才创建该折叠框，避免空折叠框
		if (!wds1.isEmpty())
		{
			propFold1 = new FoldSection("作用对象", this);
			for (int i = 0; i < wds1.size(); ++i)
			{
				propFold1->addRow(wds1[i]);
				wds1[i]->show();
			}
			m_foldWidgets.append(propFold1);
		}
		for (int i = 0; i < wds2.size(); ++i)
		{
			propFold2->addRow(wds2[i]);
			wds2[i]->show();
		}
		m_foldWidgets.append(propFold2);

		QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
		if (lastItem && lastItem->spacerItem())
		{
			_mainLayout->removeItem(lastItem);
			_mainLayout->addWidget(propFold0);
			if (propFold1)
				_mainLayout->addWidget(propFold1);
			_mainLayout->addWidget(propFold2);
			_mainLayout->addStretch();
		}
	}

	void mWorkingConditionPanel::connectSig(mLoadPanelBase* load)
	{
	    auto forcePanel = dynamic_cast<mLoadForcePanel*>(load);
	    if (forcePanel)
	    {
	        connect(forcePanel, &mLoadForcePanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onForceDefineTypeChanged);
	    }
	    auto torquePanel = dynamic_cast<mLoadTorquePanel*>(load);
	    if (torquePanel)
	    {
	        connect(torquePanel, &mLoadTorquePanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onTorqueDefineTypeChanged);
	    }
	    auto pressurePanel = dynamic_cast<mLoadPressurePanel*>(load);
	    if (pressurePanel)
	    {
	        connect(pressurePanel, &mLoadPressurePanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onPressureDefineTypeChanged);
	    }
	    auto rotatePhysicalPowerPanel = dynamic_cast<mRotatePhysicalPowerPanel*>(load);
	    if (rotatePhysicalPowerPanel)
	    {
	        connect(rotatePhysicalPowerPanel, &mRotatePhysicalPowerPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onRotatePhysicalPowerDefineTypeChanged);
	    }
	    auto surfacePanel = dynamic_cast<mLoadSurfacePanel*>(load);
	    if (surfacePanel)
	    {
	        connect(surfacePanel, &mLoadSurfacePanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onSurfaceDefineTypeChanged);
	    }
	    auto heatFluxPanel = dynamic_cast<mLoadHeatFluxPanel*>(load);
	    if (heatFluxPanel)
	    {
	        connect(heatFluxPanel, &mLoadHeatFluxPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onHeatFluxDefineTypeChanged);
	    }
	    auto radiationPanel = dynamic_cast<mLoadRadiationPanel*>(load);
	    if (radiationPanel)
	    {
	        connect(radiationPanel, &mLoadRadiationPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onRadiationDefineTypeChanged);
	    }
	    auto convectionPanel = dynamic_cast<mLoadConvectionPanel*>(load);
	    if (convectionPanel)
	    {
	        connect(convectionPanel, &mLoadConvectionPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onConvectionDefineTypeChanged);
	    }
	    load->connectUnitSystemResponse();
	}

	void mWorkingConditionPanel::disconnectSig(mLoadPanelBase* load)
	{
	    auto forcePanel = dynamic_cast<mLoadForcePanel*>(load);
	    if (forcePanel)
	    {
	        disconnect(forcePanel, &mLoadForcePanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onForceDefineTypeChanged);
	    }
	    auto torquePanel = dynamic_cast<mLoadTorquePanel*>(load);
	    if (torquePanel)
	    {
	        disconnect(torquePanel, &mLoadTorquePanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onTorqueDefineTypeChanged);
	    }
	    auto pressurePanel = dynamic_cast<mLoadPressurePanel*>(load);
	    if (pressurePanel)
	    {
	        disconnect(pressurePanel, &mLoadPressurePanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onPressureDefineTypeChanged);
	    }
	    auto rotatePhysicalPowerPanel = dynamic_cast<mRotatePhysicalPowerPanel*>(load);
	    if (rotatePhysicalPowerPanel)
	    {
	        disconnect(rotatePhysicalPowerPanel, &mRotatePhysicalPowerPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onRotatePhysicalPowerDefineTypeChanged);
	    }
	    auto surfacePanel = dynamic_cast<mLoadSurfacePanel*>(load);
	    if (surfacePanel)
	    {
	        disconnect(surfacePanel, &mLoadSurfacePanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onSurfaceDefineTypeChanged);
	    }
	    auto heatFluxPanel = dynamic_cast<mLoadHeatFluxPanel*>(load);
	    if (heatFluxPanel)
	    {
	        disconnect(heatFluxPanel, &mLoadHeatFluxPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onHeatFluxDefineTypeChanged);
	    }
	    auto radiationPanel = dynamic_cast<mLoadRadiationPanel*>(load);
	    if (radiationPanel)
	    {
	        disconnect(radiationPanel, &mLoadRadiationPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onRadiationDefineTypeChanged);
	    }
	    auto convectionPanel = dynamic_cast<mLoadConvectionPanel*>(load);
	    if (convectionPanel)
	    {
	        disconnect(convectionPanel, &mLoadConvectionPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onConvectionDefineTypeChanged);
	    }
	    load->disconnectUnitSystemResponse();
	}

	void mWorkingConditionPanel::connectSig(mBoundaryPanelBase* boundary)
	{
	    auto displacementPanel = dynamic_cast<mBoundaryDisplacementPanel*>(boundary);
	    if (displacementPanel)
	    {
	        connect(displacementPanel, &mBoundaryDisplacementPanel::sg_rebuildPanel,
	            this, &mWorkingConditionPanel::slot_onBoundaryDisplacementDefineModeChanged);
	    }
	    auto tempPanel = dynamic_cast<mBoundaryTempPanel*>(boundary);
	    if (tempPanel)
	    {
	        connect(tempPanel, &mBoundaryTempPanel::sg_rebuildPanel,
	            this, &mWorkingConditionPanel::slot_onBoundaryTempDefineTypeChanged);
	    }
	}

	void mWorkingConditionPanel::disconnectSig(mBoundaryPanelBase* boundary)
	{
	    auto displacementPanel = dynamic_cast<mBoundaryDisplacementPanel*>(boundary);
	    if (displacementPanel)
	    {
	        disconnect(displacementPanel, &mBoundaryDisplacementPanel::sg_rebuildPanel,
	            this, &mWorkingConditionPanel::slot_onBoundaryDisplacementDefineModeChanged);
	    }
	    auto tempPanel = dynamic_cast<mBoundaryTempPanel*>(boundary);
	    if (tempPanel)
	    {
	        disconnect(tempPanel, &mBoundaryTempPanel::sg_rebuildPanel,
	            this, &mWorkingConditionPanel::slot_onBoundaryTempDefineTypeChanged);
	    }
	}

	void mWorkingConditionPanel::connectSig(mPredefinedFieldPanelBase* field)
	{
		auto tempField = dynamic_cast<mPredefinedFieldTempLoadPanel*>(field);
		if (tempField)
		{
			connect(tempField, &mPredefinedFieldTempLoadPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onTempLoadDefineTypeChanged);
		}
		auto initTempField = dynamic_cast<mPredefinedFieldInitTempPanel*>(field);
		if (initTempField)
		{
			connect(initTempField, &mPredefinedFieldInitTempPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onInitTempDefineTypeChanged);
		}
	}

	void mWorkingConditionPanel::disconnectSig(mPredefinedFieldPanelBase* field)
	{
		auto tempField = dynamic_cast<mPredefinedFieldTempLoadPanel*>(field);
		if (tempField)
		{
			disconnect(tempField, &mPredefinedFieldTempLoadPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onTempLoadDefineTypeChanged);
		}
		auto initTempField = dynamic_cast<mPredefinedFieldInitTempPanel*>(field);
		if (initTempField)
		{
			disconnect(initTempField, &mPredefinedFieldInitTempPanel::sg_rebuildPanel, this, &mWorkingConditionPanel::slot_onInitTempDefineTypeChanged);
		}
	}

    void mWorkingConditionPanel::editLoad(const int& loadID, const int& caseId)
    {
        _loadBoundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadID);
        if (_loadBoundaryData == nullptr)
        {
            return;
        }
        setEditing(true);
		_editingType = EditingType::Load;

        //进入面板时，将拾取状态标志位设置为 1 或 2
        //具体情况由面板处设置，此时模型树判断标志位
        _loadBoundaryData->setPickState(1);

        //数据反显
        MechMainWindow::mLoadPanelBase* loadbase = mWorkingConditions::getInstance()->getLoadByType(_loadBoundaryData->getLoadBoundaryType());
        loadbase->setLoadData(_loadBoundaryData, caseId);
        loadbase->setInputsValue(_loadBoundaryData);
        //调出对应控件
        id = caseId;
        loadPanel(_loadBoundaryData->getLoadBoundaryType(), caseId);

        connectSig(loadbase);
    }

	void mWorkingConditionPanel::unEditLoad()
	{
		if (!isEditing() || _editingType != EditingType::Load)
		{
			return;
		}
		//入口统一复位编辑态：后续各早退分支（数据为空/已删除）无需再单独清标志
		setEditing(false);
		_editingType = EditingType::None;
		if (_loadBoundaryData == nullptr)
		{
			clearFoldWidgets();
			return;
		}
		//_loadBoundaryData 为共享指针，数据被删除（树上删除等）后引用仍非空，
		//需向管理器按 ID 复核；数据不在当前模型管理器（已删除或属于其他模型——
		//模型切换后当前管理器按 ID 查不到）时无法走数据提交收尾，但 UI 折叠框
		//仍必须卸载并复位编辑副作用，否则残留折叠框会与后续进入编辑的面板叠加显示
		bool dataAlive = false;
		if (getCVM)
		{
			auto& lbManager = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
			dataAlive = lbManager.getLoadBoundaryData(_loadBoundaryData->getLoadBoundaryId()) != nullptr;
		}
		if (!dataAlive)
		{
			//共享指针仍存活：复位拾取标志、恢复拾取工具栏，避免旧模型树符号渲染被跳过、工具栏常驻置灰
			_loadBoundaryData->setPickState(0);
			emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
			MechMainWindow::mLoadPanelBase* staleBase = mWorkingConditions::getInstance()->getLoadByType(_loadBoundaryData->getLoadBoundaryType());
			if (staleBase)
			{
				//仅断开面板重建信号，保持与 connectSig 配对；不做 deactivatePickAndCommit：
				//跨模型场景拾取提交已由拾取控件的 viewportModelAboutToChange 链路按旧模型完成，
				//此刻提交会写入当前（新）模型
				disconnectSig(staleBase);
			}
			_loadBoundaryData = nullptr;
			clearFoldWidgets();
			return;
		}
		MechMainWindow::mLoadPanelBase* loadbase = mWorkingConditions::getInstance()->getLoadByType(_loadBoundaryData->getLoadBoundaryType());
		if (loadbase == nullptr)
		{
			_loadBoundaryData->setPickState(0);
			clearFoldWidgets();
		    return;
		}

        //切出面板时，将拾取状态标志位设置为0
        //此时模型树判断集合映射
        _loadBoundaryData->setPickState(0);

        // 切出面板：恢复拾取工具栏全部按钮，避免常驻置灰影响其它面板
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));

		// 切换/退出面板前，先让拾取控件失活提交当前数据（面板为单例复用，不走析构）。
		// 此时 _loadData 尚未被覆盖、信号仍连接，是失活 commit 的正确时机。
        loadbase->deactivatePickAndCommit();
		disconnectSig(loadbase);
        //退出载荷编辑界面，通知面板收尾（如压强云图关闭）
        loadbase->unEditInputs();
		clearFoldWidgets();
	}

	void mWorkingConditionPanel::editBoundary(const int& boundaryID, const int& caseId)
	{
		_loadBoundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(boundaryID);
		if (_loadBoundaryData == nullptr)
		{
			return;
		}
		setEditing(true);
		_editingType = EditingType::Boundary;

        //进入面板时，将拾取状态标志位设置为 1 或 2
        //具体情况由面板处设置，此时模型树判断标志位
        _loadBoundaryData->setPickState(1);

		//数据反显
		MechMainWindow::mBoundaryPanelBase* boundarybase = mWorkingConditions::getInstance()->getBoundaryByType(_loadBoundaryData->getLoadBoundaryType());
		boundarybase->setBoundaryData(_loadBoundaryData, caseId);
		boundarybase->setInputsValue(_loadBoundaryData);
		//调出对应控件
		      boundaryPanel(_loadBoundaryData->getLoadBoundaryType(), caseId);

		connectSig(boundarybase);
	}

    void mWorkingConditionPanel::unEditBoundary()
    {
        if (!isEditing() || _editingType != EditingType::Boundary)
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支（数据为空/已删除）无需再单独清标志
        setEditing(false);
		_editingType = EditingType::None;
        if (_loadBoundaryData == nullptr)
        {
            clearFoldWidgets();
            return;
        }
        //_loadBoundaryData 为共享指针，数据被删除（树上删除等）后引用仍非空，
        //需向管理器按 ID 复核；数据不在当前模型管理器（已删除或属于其他模型——
        //模型切换后当前管理器按 ID 查不到）时无法走数据提交收尾，但 UI 折叠框
        //仍必须卸载并复位编辑副作用，否则残留折叠框会与后续进入编辑的面板叠加显示
        bool dataAlive = false;
        if (getCVM)
        {
            auto& lbManager = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
            dataAlive = lbManager.getLoadBoundaryData(_loadBoundaryData->getLoadBoundaryId()) != nullptr;
        }
        if (!dataAlive)
        {
            //共享指针仍存活：复位拾取标志、恢复拾取工具栏
            _loadBoundaryData->setPickState(0);
            emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
            MechMainWindow::mBoundaryPanelBase* staleBase = mWorkingConditions::getInstance()->getBoundaryByType(_loadBoundaryData->getLoadBoundaryType());
            if (staleBase)
            {
                //仅断开面板重建信号，保持与 connectSig 配对；不做 deactivatePickAndCommit：
                //跨模型场景拾取提交已由拾取控件的 viewportModelAboutToChange 链路按旧模型完成
                disconnectSig(staleBase);
            }
            _loadBoundaryData = nullptr;
            clearFoldWidgets();
            return;
        }
        MechMainWindow::mBoundaryPanelBase* boundarybase = mWorkingConditions::getInstance()->getBoundaryByType(_loadBoundaryData->getLoadBoundaryType());
        if (boundarybase == nullptr)
        {
            _loadBoundaryData->setPickState(0);
            clearFoldWidgets();
            return;
        }
        //切出面板时，将拾取状态标志位设置为0
        //此时模型树判断集合映射
        _loadBoundaryData->setPickState(0);
        // 切出面板：恢复拾取工具栏全部按钮，避免常驻置灰影响其它面板
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
        // 固定约束/位移转角/虚拟约束/超单元/温度边界面板：切出前失活并提交通用拾取控件数据
        boundarybase->deactivatePickAndCommit();
        disconnectSig(boundarybase);
        clearFoldWidgets();
    }

    void mWorkingConditionPanel::editPredefinedField(const int& fieldID, const int& caseId)
    {
		_predefinedFieldData = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(fieldID);
		if (_predefinedFieldData == nullptr)
		{
			return;
		}
		setEditing(true);
		_editingType = EditingType::PredefinedField;
		//进入面板时，将拾取状态标志位设置为 1（面板编辑中），
        //模型树据此跳过 showPredefinedField，由面板实时渲染负责符号，避免双重渲染
        _predefinedFieldData->setPickState(1);

        //数据反显
		MechMainWindow::mPredefinedFieldPanelBase* fieldbase = mWorkingConditions::getInstance()->getPredefinedFieldByType(_predefinedFieldData->getPredefinedFieldType());
		fieldbase->setPredefinedFieldData(_predefinedFieldData, caseId);
		fieldbase->setInputsValue(_predefinedFieldData);
		//调出对应控件
        id = caseId;
        predefinedFieldPanel(_predefinedFieldData->getPredefinedFieldType(), caseId);

		connectSig(fieldbase);
	}

	void mWorkingConditionPanel::unEditPredefinedField()
	{
		if (!isEditing() || _editingType != EditingType::PredefinedField)
		{
			return;
		}
		//入口统一复位编辑态：后续各早退分支（数据为空/已删除）无需再单独清标志
		setEditing(false);
		_editingType = EditingType::None;
		if (_predefinedFieldData == nullptr)
		{
			clearFoldWidgets();
			return;
		}
		//_predefinedFieldData 为共享指针，数据被删除（树上删除等）后引用仍非空，
		//需向管理器按 ID 复核；数据不在当前模型管理器（已删除或属于其他模型——
		//模型切换后当前管理器按 ID 查不到）时无法走数据提交收尾，但 UI 折叠框
		//仍必须卸载并复位编辑副作用，否则残留折叠框会与后续进入编辑的面板叠加显示
		bool dataAlive = false;
		if (getCVM)
		{
			auto& fieldManager = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField);
			dataAlive = fieldManager.getPredefinedFieldData(_predefinedFieldData->getPredefinedFieldId()) != nullptr;
		}
		if (!dataAlive)
		{
			//共享指针仍存活：复位拾取标志、恢复拾取工具栏
			_predefinedFieldData->setPickState(0);
			emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
			MechMainWindow::mPredefinedFieldPanelBase* staleBase = mWorkingConditions::getInstance()->getPredefinedFieldByType(_predefinedFieldData->getPredefinedFieldType());
			if (staleBase)
			{
				//仅断开面板重建信号，保持与 connectSig 配对；不做 deactivatePickAndCommit：
				//跨模型场景拾取提交已由拾取控件的 viewportModelAboutToChange 链路按旧模型完成
				disconnectSig(staleBase);
			}
			_predefinedFieldData = nullptr;
			clearFoldWidgets();
			return;
		}
		MechMainWindow::mPredefinedFieldPanelBase* fieldbase = mWorkingConditions::getInstance()->getPredefinedFieldByType(_predefinedFieldData->getPredefinedFieldType());
		if (fieldbase == nullptr)
		{
			_predefinedFieldData->setPickState(0);
			clearFoldWidgets();
			return;
		}

        //切出面板时，将拾取状态标志位复位为 0（未编辑），
        //模型树此后对该预定义场的 showPredefinedField 恢复正常执行
        _predefinedFieldData->setPickState(0);

        // 切出面板：恢复拾取工具栏全部按钮，避免常驻置灰影响其它面板
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
		// 温度载荷面板使用通用拾取控件，切换/退出面板前需失活提交当前拾取数据
        fieldbase->deactivatePickAndCommit();
		disconnectSig(fieldbase);
		clearFoldWidgets();
	}
	
	void mWorkingConditionPanel::createRotateAnalyse(MxFunctions::RotateAnalysyType type)
	{
		mRotateAnalyseManager& manager = getCVM->getModelData()->getManager<mRotateAnalyseManager>(Physical_RotateAnalyse);
		_rotateAnalyseData = nullptr;

		switch (type)
		{
		case ROTATIONAL_ACC:
		{
			auto newItem = std::make_shared<MechCaseData::mLoadRotationalACC>(manager.createID(), ROTATIONAL_ACC, getNewRotateAnalyseName("RotationalAcceleration"));
			_rotateAnalyseData = newItem;
		}
		break;
		case ROTATIONAL_SPEED:
		{
			auto newItem = std::make_shared<MechCaseData::mLoadRotationalSpeed>(manager.createID(), ROTATIONAL_SPEED, getNewRotateAnalyseName("RotationalSpeed"));
			_rotateAnalyseData = newItem;
		}
		break;
		default:
			return;
		}

		getCVM->getModelData()->addPhysicalFieldP(_rotateAnalyseData);
		setEditing(true);
		_editingType = EditingType::RotateAnalyse;

		mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建旋转分析 %1").arg(_rotateAnalyseData->getRotateAnalyseName()), true);
		emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRotateAnalyseSig(true, _rotateAnalyseData->getRotateAnalyseId());
	}

	void mWorkingConditionPanel::editRotateAnalyse(const int& rotateAnalyseID, const int& caseId)
	{
		_rotateAnalyseData = getCVM->getModelData()->getManager<mRotateAnalyseManager>(Physical_RotateAnalyse).getRotateAnalyseData(rotateAnalyseID);
		if (_rotateAnalyseData == nullptr)
		{
			return;
		}
		setEditing(true);
		_editingType = EditingType::RotateAnalyse;
		MechMainWindow::mRotateAnalysePanelBase* rotatebase = mWorkingConditions::getInstance()->getRotateAnalyseByType(_rotateAnalyseData->getRotateAnalyseType());
		rotatebase->setRotateAnalyseData(_rotateAnalyseData, caseId);
		rotatebase->setInputsValue(_rotateAnalyseData);
		id = caseId;
		rotateAnalysePanel(_rotateAnalyseData->getRotateAnalyseType(), caseId);
	}

	void mWorkingConditionPanel::unEditRotateAnalyse()
	{
		if (!isEditing() || _editingType != EditingType::RotateAnalyse)
		{
			return;
		}
		//入口统一复位编辑态：后续各早退分支（数据为空/已删除）无需再单独清标志
		setEditing(false);
		_editingType = EditingType::None;
		if (_rotateAnalyseData == nullptr)
		{
			clearFoldWidgets();
			return;
		}
		//_rotateAnalyseData 为共享指针，数据被删除（树上删除等）后引用仍非空，
		//需向管理器按 ID 复核；数据不在当前模型管理器（已删除或属于其他模型——
		//模型切换后当前管理器按 ID 查不到）时无法回存面板值，但 UI 折叠框
		//仍必须卸载，否则残留折叠框会与后续进入编辑的面板叠加显示
		bool dataAlive = false;
		if (getCVM)
		{
			auto& rotateManager = getCVM->getModelData()->getManager<mRotateAnalyseManager>(Physical_RotateAnalyse);
			dataAlive = rotateManager.getRotateAnalyseData(_rotateAnalyseData->getRotateAnalyseId()) != nullptr;
		}
		if (!dataAlive)
		{
			_rotateAnalyseData = nullptr;
			clearFoldWidgets();
			return;
		}
		MechMainWindow::mRotateAnalysePanelBase* rotatebase = mWorkingConditions::getInstance()->getRotateAnalyseByType(_rotateAnalyseData->getRotateAnalyseType());
		if (rotatebase == nullptr)
		{
			clearFoldWidgets();
			return;
		}
		// 切出前将面板内部数据同步到外部数据对象
		switch (_rotateAnalyseData->getRotateAnalyseType())
		{
		case ROTATIONAL_ACC:
		{
			auto rotatepanel = dynamic_cast<mLoadRotationalACCPanel*>(rotatebase);
			auto rotatePtr = std::dynamic_pointer_cast<MechCaseData::mLoadRotationalACC>(_rotateAnalyseData);
			if (rotatepanel && rotatePtr)
				rotatepanel->storeLoadValue(rotatePtr);
		}
		break;
		case ROTATIONAL_SPEED:
		{
			auto rotatepanel = dynamic_cast<mLoadRotationalSpeedPanel*>(rotatebase);
			auto rotatePtr = std::dynamic_pointer_cast<MechCaseData::mLoadRotationalSpeed>(_rotateAnalyseData);
			if (rotatepanel && rotatePtr)
				rotatepanel->storeLoadValue(rotatePtr);
		}
		break;
		default:
			break;
		}
		clearFoldWidgets();
	}

	mMixSet* mWorkingConditionPanel::createSet()
	{
		int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
		mMixSet* eleset = new mMixSet(setID, SetType::Mix, "contain", true);
		getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
		return eleset;
	}

	QString mWorkingConditionPanel::getNewLoadName(const QString& loadname)
	{
		auto&manager = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary);
		int id = 1;
		QString name = loadname + "_" + MxFunctions::mNumberToString(id);
		while (manager.ExistLoadBoundaryName(name))
		{
			name = loadname + "_" + MxFunctions::mNumberToString(++id);
		}
		return name;
	}

	QString mWorkingConditionPanel::getNewBoundaryName(const QString& boundaryname)
	{
		auto& manager = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary);
		int id = 1;
		QString name = boundaryname + "_" + MxFunctions::mNumberToString(id);
		while (manager.ExistLoadBoundaryName(name))
		{
			name = boundaryname + "_" + MxFunctions::mNumberToString(++id);
		}
		return name;
	}

	QString mWorkingConditionPanel::getNewPredefinedFieldName(const QString& fieldname)
	{
		auto& manager = getCVM->getModelData()->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField);
		int id = 1;
		QString name = fieldname + "_" + MxFunctions::mNumberToString(id);
		while (manager.ExistPredefinedFieldName(name))
		{
			name = fieldname + "_" + MxFunctions::mNumberToString(++id);
		}
		return name;
	}
	
    QString mWorkingConditionPanel::getNewRotateAnalyseName(const QString& rotateAnalyseName)
    {
        auto& manager = getCVM->getModelData()->getManager<MechCaseData::mRotateAnalyseManager>(Physical_RotateAnalyse);
        int id = 1;
        QString name = rotateAnalyseName + "_" + MxFunctions::mNumberToString(id);
        while (manager.ExistRotateAnalyseName(name))
        {
            name = rotateAnalyseName + "_" + MxFunctions::mNumberToString(++id);
        }
        return name;
    }

    void mWorkingConditionPanel::slot_onTempLoadDefineTypeChanged()
    {
        if (_predefinedFieldData == nullptr)
        {
            return;
        }
        // 温度载荷面板已改为局部保存，数据由控件自身实时保存；
        // 此槽仅负责在定义方式切换导致折叠框数量变化时重建面板。
        rebuildTempLoadPanel();
    }

    void mWorkingConditionPanel::slot_onInitTempDefineTypeChanged()
    {
        if (_predefinedFieldData == nullptr)
        {
            return;
        }
        // 初始温度面板数据由控件自身实时保存；
        // 独立赋值不显示作用对象折叠框，定义方式切换导致折叠框数量变化，需重建面板。
        rebuildTempLoadPanel();
    }

	void mWorkingConditionPanel::slot_onForceDefineTypeChanged()
	{
		if (_loadBoundaryData == nullptr)
		{
			return;
		}
		// 数据由控件自身实时保存，重建面板即可
		rebuildLoadPanel();
	}

	void mWorkingConditionPanel::slot_onTorqueDefineTypeChanged()
	{
		if (_loadBoundaryData == nullptr)
		{
			return;
		}
		// 数据由控件自身实时保存，重建面板即可
		rebuildLoadPanel();
	}

	void mWorkingConditionPanel::slot_onPressureDefineTypeChanged()
	{
		if (_loadBoundaryData == nullptr)
		{
			return;
		}
		// 数据由控件自身实时保存，重建面板即可
		rebuildLoadPanel();
	}

	void mWorkingConditionPanel::slot_onRotatePhysicalPowerDefineTypeChanged()
	{
		if (_loadBoundaryData == nullptr)
		{
			return;
		}
		// 数据由控件自身实时保存，重建面板即可
		rebuildLoadPanel();
	}

	void mWorkingConditionPanel::slot_onSurfaceDefineTypeChanged()
	{
		if (_loadBoundaryData == nullptr)
		{
			return;
		}
		// 数据由控件自身实时保存，重建面板即可
		rebuildLoadPanel();
	}

	void mWorkingConditionPanel::slot_onHeatFluxDefineTypeChanged()
	{
	    if (_loadBoundaryData == nullptr)
	    {
	        return;
	    }
	    // 数据由控件自身实时保存，重建面板即可
	    rebuildLoadPanel();
	}

	void mWorkingConditionPanel::slot_onRadiationDefineTypeChanged()
	{
	    if (_loadBoundaryData == nullptr)
	    {
	        return;
	    }
	    // 数据由控件自身实时保存，重建面板即可
	    rebuildLoadPanel();
	}

    void mWorkingConditionPanel::slot_onConvectionDefineTypeChanged()
    {
        if (_loadBoundaryData == nullptr)
        {
            return;
        }
        // 数据由控件自身实时保存，重建面板即可
        rebuildLoadPanel();
    }

    void mWorkingConditionPanel::slot_onBoundaryDisplacementDefineModeChanged()
    {
        if (_loadBoundaryData == nullptr)
        {
            return;
        }
        // 数据由控件自身实时保存，重建面板即可
        rebuildBoundaryPanel();
    }

    void mWorkingConditionPanel::slot_onBoundaryTempDefineTypeChanged()
    {
        if (_loadBoundaryData == nullptr)
        {
            return;
        }
        // 数据由控件自身实时保存，重建面板即可
        rebuildBoundaryPanel();
    }

    void mWorkingConditionPanel::rebuildBoundaryPanel()
    {
        if (_loadBoundaryData == nullptr)
        {
            return;
        }
        // 断开信号
        MechMainWindow::mBoundaryPanelBase* boundarybase = mWorkingConditions::getInstance()->getBoundaryByType(_loadBoundaryData->getLoadBoundaryType());
        if (boundarybase == nullptr)
        {
            return;
        }
        disconnectSig(boundarybase);

        // 移除旧折叠块
        for (auto foldwd : m_foldWidgets)
        {
            _mainLayout->removeWidget(foldwd);
            auto foldsection = dynamic_cast<FoldSection*>(foldwd);
            foldsection->removeAll();
            delete foldwd;
            foldwd = nullptr;
        }
        m_foldWidgets.clear();

        // 重新创建面板
        boundaryPanel(_loadBoundaryData->getLoadBoundaryType(), id);

        // 重新连接信号
        connectSig(boundarybase);
    }

	void mWorkingConditionPanel::rebuildLoadPanel()
	{
		if (_loadBoundaryData == nullptr)
		{
			return;
		}
		// 断开信号
		MechMainWindow::mLoadPanelBase* loadbase = mWorkingConditions::getInstance()->getLoadByType(_loadBoundaryData->getLoadBoundaryType());
		if (loadbase == nullptr)
		{
			return;
		}
		disconnectSig(loadbase);

		// 移除旧折叠块
		for (auto foldwd : m_foldWidgets)
		{
			_mainLayout->removeWidget(foldwd);
			auto foldsection = dynamic_cast<FoldSection*>(foldwd);
			foldsection->removeAll();
			delete foldwd;
			foldwd = nullptr;
		}
		m_foldWidgets.clear();

		// 重新创建面板
		loadPanel(_loadBoundaryData->getLoadBoundaryType(), id);

		// 重新连接信号
		connectSig(loadbase);
	}

	void mWorkingConditionPanel::rebuildTempLoadPanel()
	{
		if (_predefinedFieldData == nullptr)
		{
			return;
		}
		// 断开信号
		MechMainWindow::mPredefinedFieldPanelBase* fieldbase = mWorkingConditions::getInstance()->getPredefinedFieldByType(_predefinedFieldData->getPredefinedFieldType());
		if (fieldbase == nullptr)
		{
			return;
		}
		disconnectSig(fieldbase);

		// 移除旧折叠块
		for (auto foldwd : m_foldWidgets)
		{
			_mainLayout->removeWidget(foldwd);
			auto foldsection = dynamic_cast<FoldSection*>(foldwd);
			foldsection->removeAll();
			delete foldwd;
			foldwd = nullptr;
		}
		m_foldWidgets.clear();

		// 重新创建面板
		predefinedFieldPanel(_predefinedFieldData->getPredefinedFieldType(),id);

		// 重新连接信号
		connectSig(fieldbase);
	}
}
