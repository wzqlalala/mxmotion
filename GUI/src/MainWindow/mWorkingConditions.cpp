#include "mWorkingConditions.h"

namespace MechMainWindow
{
    mWorkingConditions* mWorkingConditions::_instance = nullptr;

    mWorkingConditions* mWorkingConditions::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mWorkingConditions;
        }
        return _instance;
    }

    mWorkingConditions::mWorkingConditions(QObject* parent)
        : QObject(parent)
    {}

    mWorkingConditions::~mWorkingConditions()
    {
        delete m_loadForce;
        delete m_LoadTorque;
        delete m_LoadPressure;
        delete m_LoadGravity;
        delete m_LoadHeatFlux;
        delete m_LoadRadiation;
        delete m_LoadConvection;
        delete m_LoadTransient;
        delete m_LoadFreqdomain;
        delete m_LoadCombined;
        delete m_LoadBoltPanel;
        delete m_LoadAcousticConcForcePanel;
        delete m_LoadAcousticFrequencyDomainPanel;
        delete m_LoadSpectrum;
        delete m_LoadRotatePhysicalPower;
        delete m_LoadSurface;
        delete m_LoadEdge;
        delete m_BoundaryFixed;
        delete m_BoundaryDisplacement;
        delete m_BoundaryTemp;
        delete m_BoundaryVirtual;
        delete m_BoundarySuperelement;
        delete m_BoundaryMove;
        delete m_BoundaryTempNew;
        delete m_PredefinedFieldInitTemp;
        delete m_PredefinedFieldTempLoad;
        delete m_LoadRotationalACCPanel;
        delete m_LoadRotationalSpeedPanel;
    }

    mLoadPanelBase* mWorkingConditions::getLoadByType(MxFunctions::LoadBoundaryType type)
    {
        m_loadBase = nullptr;
        switch (type)
        {
        case LOAD_IM_FORCE:
            if (!m_loadForce) m_loadForce = new mLoadForcePanel();
            m_loadBase = m_loadForce;
            break;
        case LOAD_IM_TORQUE:
            if (!m_LoadTorque) m_LoadTorque = new mLoadTorquePanel();
            m_loadBase = m_LoadTorque;
            break;
        case LOAD_IM_PRESSURE:
            if (!m_LoadPressure) m_LoadPressure = new mLoadPressurePanel();
            m_loadBase = m_LoadPressure;
            break;
        case LOAD_IM_GRAVITY:
            if (!m_LoadGravity) m_LoadGravity = new mLoadGravityPanel();
            m_loadBase = m_LoadGravity;
            break;
        case LOAD_IM_HEATFLUX:
            if (!m_LoadHeatFlux) m_LoadHeatFlux = new mLoadHeatFluxPanel();
            m_loadBase = m_LoadHeatFlux;
            break;
        case LOAD_IM_RADIATION:
            if (!m_LoadRadiation) m_LoadRadiation = new mLoadRadiationPanel();
            m_loadBase = m_LoadRadiation;
            break;
        case LOAD_IM_CONVECTION:
            if (!m_LoadConvection) m_LoadConvection = new mLoadConvectionPanel();
            m_loadBase = m_LoadConvection;
            break;
        case LOAD_IM_TRANSIENT:
            if (!m_LoadTransient) m_LoadTransient = new mLoadTransientPanel();
            m_loadBase = m_LoadTransient;
            break;
        case LOAD_IM_FREQDOMAIN:
            if (!m_LoadFreqdomain) m_LoadFreqdomain = new mLoadFreqdomainPanel();
            m_loadBase = m_LoadFreqdomain;
            break;
        case LOAD_IM_COMBINED:
            if (!m_LoadCombined) m_LoadCombined = new mLoadCombinedPanel();
            m_loadBase = m_LoadCombined;
            break;
        case LOAD_IM_BOLT:
            if (!m_LoadBoltPanel) m_LoadBoltPanel = new mLoadBoltPanel();
            m_loadBase = m_LoadBoltPanel;
            break;
        case LOAD_IM_ACOCONCFORCE:
            if (!m_LoadAcousticConcForcePanel) m_LoadAcousticConcForcePanel = new mLoadAcousticConcForcePanel();
            m_loadBase = m_LoadAcousticConcForcePanel;
            break;
        case LOAD_IM_ACOFREQDOMAIN:
            if (!m_LoadAcousticFrequencyDomainPanel) m_LoadAcousticFrequencyDomainPanel = new mLoadAcousticFrequencyDomainPanel();
            m_loadBase = m_LoadAcousticFrequencyDomainPanel;
            break;
        case LOAD_IM_SPECTRUM:
            if (!m_LoadSpectrum) m_LoadSpectrum = new mLoadSpectrumPanel();
            m_loadBase = m_LoadSpectrum;
            break;
        case LOAD_ROTATE_PHYSICAL_POWER:
            if (!m_LoadRotatePhysicalPower) m_LoadRotatePhysicalPower = new mRotatePhysicalPowerPanel();
            m_loadBase = m_LoadRotatePhysicalPower;
            break;
        case LOAD_IM_SURFACE:
            if (!m_LoadSurface) m_LoadSurface = new mLoadSurfacePanel();
            m_loadBase = m_LoadSurface;
            break;
        case LOAD_IM_EDGE:
            if (!m_LoadEdge) m_LoadEdge = new mLoadEdgePanel();
            m_loadBase = m_LoadEdge;
            break;
        default:
            break;
        }
        return m_loadBase;
    }

    mBoundaryPanelBase* mWorkingConditions::getBoundaryByType(MxFunctions::LoadBoundaryType type)
    {
        m_boundaryBase = nullptr;
        switch (type)
        {
        case BOUNDARY_IM_FIXED:
            if (!m_BoundaryFixed) m_BoundaryFixed = new mBoundaryFixedPanel();
            m_boundaryBase = m_BoundaryFixed;
            break;
        case BOUNDARY_IM_DISPLACEMENT:
        case BOUNDARY_IM_SPEED:
        case BOUNDARY_IM_ACC:
            if (!m_BoundaryDisplacement) m_BoundaryDisplacement = new mBoundaryDisplacementPanel();
            m_boundaryBase = m_BoundaryDisplacement;
            break;
        case BOUNDARY_IM_TEMP:
            if (!m_BoundaryTemp) m_BoundaryTemp = new mBoundaryTempPanel();
            m_boundaryBase = m_BoundaryTemp;
            break;
        case BOUNDARY_IM_VIRTUAL:
            if (!m_BoundaryVirtual) m_BoundaryVirtual = new mBoundaryVirtualPanel();
            m_boundaryBase = m_BoundaryVirtual;
            break;
        case BOUNDARY_IM_SUPERELEMENT:
            if (!m_BoundarySuperelement) m_BoundarySuperelement = new mBoundarySuperelementPanel();
            m_boundaryBase = m_BoundarySuperelement;
            break;
        case BOUNDARY_MOVE:
            if (!m_BoundaryMove) m_BoundaryMove = new mBoundaryMovePanel();
            m_boundaryBase = m_BoundaryMove;
            break;
        case BOUNDARY_TEMP_NEW:
            if (!m_BoundaryTempNew) m_BoundaryTempNew = new mBoundaryTempNewPanel();
            m_boundaryBase = m_BoundaryTempNew;
            break;
        default:
            break;
        }
        return m_boundaryBase;
    }

    mPredefinedFieldPanelBase* mWorkingConditions::getPredefinedFieldByType(MxFunctions::PredefinedFieldType type)
    {
        m_predefinedFieldBase = nullptr;
        switch (type)
        {
        case PREDEFINED_FIELD_INIT_TEMP:
            if (!m_PredefinedFieldInitTemp) m_PredefinedFieldInitTemp = new mPredefinedFieldInitTempPanel();
            m_predefinedFieldBase = m_PredefinedFieldInitTemp;
            break;
        case PREDEFINED_FIELD_TEMP_LOAD:
            if (!m_PredefinedFieldTempLoad) m_PredefinedFieldTempLoad = new mPredefinedFieldTempLoadPanel();
            m_predefinedFieldBase = m_PredefinedFieldTempLoad;
            break;
        default:
            break;
        }
        return m_predefinedFieldBase;
    }

    mRotateAnalysePanelBase* mWorkingConditions::getRotateAnalyseByType(MxFunctions::RotateAnalysyType type)
    {
        m_rotateAnalyseBase = nullptr;
        switch (type)
        {
        case ROTATIONAL_ACC:
            if (!m_LoadRotationalACCPanel) m_LoadRotationalACCPanel = new mLoadRotationalACCPanel();
            m_rotateAnalyseBase = m_LoadRotationalACCPanel;
            break;
        case ROTATIONAL_SPEED:
            if (!m_LoadRotationalSpeedPanel) m_LoadRotationalSpeedPanel = new mLoadRotationalSpeedPanel();
            m_rotateAnalyseBase = m_LoadRotationalSpeedPanel;
            break;
        default:
            break;
        }
        return m_rotateAnalyseBase;
    }
}
