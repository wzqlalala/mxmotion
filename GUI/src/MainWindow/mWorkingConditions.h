#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <QObject>
#include <memory>

#include "mLoadPanelBase.h"
#include "mLoadForcePanel.h"
#include "mLoadTorquePanel.h"
#include "mLoadPressurePanel.h"
#include "mLoadGravityPanel.h"

#include "mLoadHeatFluxPanel.h"
#include "mLoadRadiationPanel.h"
#include "mLoadConvectionPanel.h"

#include "mLoadTransientPanel.h"
#include "mLoadFreqdomainPanel.h"
#include "mLoadCombinedPanel.h"
#include "mLoadSpectrumPanel.h"
#include "mLoadRotationalACCPanel.h"
#include "mLoadRotationalSpeedPanel.h"
#include "mRotatePhysicalPowerPanel.h"
#include "mRotateAnalysePanelBase.h"
#include "mLoadSurfacePanel.h"
#include "mLoadEdgePanel.h"

#include "mLoadBoltPanel.h"
#include "mLoadAcousticConcForcePanel.h"
#include "mLoadAcousticFrequencyDomainPanel.h"
#include "mBoundaryPanelBase.h"

#include "mBoundaryFixedPanel.h"
#include "mBoundaryDisplacementPanel.h"
#include "mBoundaryTempPanel.h"
#include "mBoundaryVirtualPanel.h"
#include "mBoundarySuperelementPanel.h"
#include "mBoundaryMovePanel.h"
#include "mBoundaryTempNewPanel.h"

#include "mPredefinedFieldPanelBase.h"
#include "mPredefinedFieldInitTempPanel.h"
#include "mPredefinedFieldTempLoadPanel.h"

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mWorkingConditions : public QObject
	{
		Q_OBJECT

	public:		
		/*
		* 析构函数
		*/
		~mWorkingConditions();

		/*
		* 获取单例指针
		*/
		static mWorkingConditions* getInstance();

		/*
		* 根据类型获取载荷控件
		*/
        mLoadPanelBase* getLoadByType(MxFunctions::LoadBoundaryType type);

		/*
		* 根据类型获取边界控件
		*/
        mBoundaryPanelBase* getBoundaryByType(MxFunctions::LoadBoundaryType type);

		/*
		* 根据类型获取预定义场控件
		*/
		mPredefinedFieldPanelBase* getPredefinedFieldByType(MxFunctions::PredefinedFieldType type);

		/*
		* 根据类型获取旋转分析控件
		*/
		mRotateAnalysePanelBase* getRotateAnalyseByType(MxFunctions::RotateAnalysyType type);

	private:
		/*
		* 构造函数
		*/
		mWorkingConditions(QObject* parent = nullptr);

	private:
		//单例指针
		static mWorkingConditions* _instance;

        mLoadPanelBase* m_loadBase = nullptr;
		mLoadForcePanel* m_loadForce = nullptr;
		mLoadTorquePanel* m_LoadTorque = nullptr;
		mLoadPressurePanel* m_LoadPressure = nullptr;
        mLoadGravityPanel* m_LoadGravity = nullptr;

        mLoadHeatFluxPanel* m_LoadHeatFlux = nullptr;
        mLoadRadiationPanel* m_LoadRadiation = nullptr;
        mLoadConvectionPanel* m_LoadConvection = nullptr;

        mLoadTransientPanel* m_LoadTransient = nullptr;
        mLoadFreqdomainPanel* m_LoadFreqdomain = nullptr;
        mLoadCombinedPanel* m_LoadCombined = nullptr;
        mLoadBoltPanel* m_LoadBoltPanel = nullptr;
        mLoadAcousticConcForcePanel* m_LoadAcousticConcForcePanel = nullptr;
        mLoadAcousticFrequencyDomainPanel* m_LoadAcousticFrequencyDomainPanel = nullptr;
        mLoadSpectrumPanel* m_LoadSpectrum = nullptr;//谱载荷面板

        mLoadRotationalACCPanel* m_LoadRotationalACCPanel = nullptr; //旋转加速度面板
        mLoadRotationalSpeedPanel* m_LoadRotationalSpeedPanel = nullptr; //旋转速度面板
        mRotatePhysicalPowerPanel* m_LoadRotatePhysicalPower = nullptr; //旋转体力面板
        mLoadSurfacePanel* m_LoadSurface= nullptr; //旋转体力面板
        mLoadEdgePanel* m_LoadEdge = nullptr; //边载荷面板

        mBoundaryPanelBase* m_boundaryBase = nullptr;
        mBoundaryFixedPanel* m_BoundaryFixed = nullptr;
        mBoundaryDisplacementPanel* m_BoundaryDisplacement = nullptr;
		mBoundaryTempPanel* m_BoundaryTemp = nullptr;
        mBoundaryVirtualPanel* m_BoundaryVirtual = nullptr;
        mBoundarySuperelementPanel* m_BoundarySuperelement = nullptr;
        mBoundaryMovePanel* m_BoundaryMove = nullptr;
        mBoundaryTempNewPanel* m_BoundaryTempNew = nullptr;

		mPredefinedFieldPanelBase* m_predefinedFieldBase = nullptr;
		mPredefinedFieldInitTempPanel* m_PredefinedFieldInitTemp = nullptr;
		mPredefinedFieldTempLoadPanel* m_PredefinedFieldTempLoad = nullptr;

		mRotateAnalysePanelBase* m_rotateAnalyseBase = nullptr;
	};
}
