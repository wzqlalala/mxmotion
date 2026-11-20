#pragma once
// 双 ModelData 树同源共享头：MxAI 工作区同时安装 RebuildModelData(dev_Mech) 与
// RebuildModelData_Dyna(dev_Dyna)，本文件两树逐字节同源；#pragma once 按路径去重
// 无法跨树去重（双份均被包含即 C2011 重定义），补同名 include guard 先含者胜。
#ifndef SAMPLE2D_MARKERS_H
#define SAMPLE2D_MARKERS_H

//【OCC迁移·最优收敛】休眠显示类整类已注释，配套 include 一并注释——
// 本头由此零 OCC（活跃 .hxx 归零判据）；如需唤醒类请一并恢复。
//#include <Standard_Macro.hxx>
//#include <Standard_DefineHandle.hxx>
//#include <Graphic3d_ArrayOfPoints.hxx>
//#include <AIS_InteractiveObject.hxx>

namespace MxBeam
{
	//OA dormant display class
//class Sample2D_Markers : public AIS_InteractiveObject
//	{
//		enum Sample2D_CurrentTypeOfMarker {
//			Sample2D_CTOM_Generic,
//			Sample2D_CTOM_Polyline,
//			Sample2D_CTOM_Circle,
//			Sample2D_CTOM_Ellips
//		};

//	public:
//		// Generic marker
//		Standard_EXPORT Sample2D_Markers(const double theXPosition,
//			const double theYPosition,
//			const Aspect_TypeOfMarker theMarkerType,
//			const Quantity_Color theColor,
//			const double theScaleOrId = 5.0);
//		// Polyline marker
//		Standard_EXPORT Sample2D_Markers(const double theXPosition,
//			const double theYPosition,
//			const Handle(Graphic3d_ArrayOfPoints)& theArrayOfPoints,
//			const Aspect_TypeOfMarker theMarkerType,
//			const Quantity_Color theColor,
//			const double theScaleOrId = 2.0);
//		DEFINE_STANDARD_RTTIEXT(Sample2D_Markers, AIS_InteractiveObject)
//	private:
//#ifdef OCC7_5_0
//		virtual void Compute(const Handle(PrsMgr_PresentationManager3d)& /*aPresentationManager*/,
//			const Handle(Prs3d_Presentation)& /*aPresentation*/,
//			const Standard_Integer /*aMode*/);
//#elif defined(OCC7_9_0)
//		void Compute(const Handle(PrsMgr_PresentationManager)&,
//			const Handle(Prs3d_Presentation)& aPresentation,
//			const Standard_Integer);
		
//#endif 


//		virtual void ComputeSelection(const Handle(SelectMgr_Selection)& /*aSelection*/,
//			const Standard_Integer /*aMode*/) {}


//		Standard_EXPORT virtual void SetContext(const Handle(AIS_InteractiveContext)& /*theContext*/) {}



//		Sample2D_CurrentTypeOfMarker myCurrentTypeOfMarker;
//		double  myXPosition;
//		double  myYPosition;
//		Aspect_TypeOfMarker myMarkerType;
//		Quantity_Color myColor;
//		double  myWidth;
//		double  myHeight;
//		double  myIndex;//myScaleOrId
//		// specific polyline marker
//		Handle(Graphic3d_ArrayOfPoints) myArrayOfPoints;
//	};

}
#endif // SAMPLE2D_MARKERS_H
