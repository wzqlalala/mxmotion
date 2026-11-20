//OA dormant impl
//#include "Sample2D_Markers.h"
//#include "Prs3d_Root.hxx"
//namespace MxBeam
//{
//	IMPLEMENT_STANDARD_RTTIEXT(Sample2D_Markers, AIS_InteractiveObject)

	

	// generic marker
//	Sample2D_Markers::Sample2D_Markers(const double theXPosition, const double theYPosition,
//		const Aspect_TypeOfMarker theMarkerType, const Quantity_Color theColor, const double theScaleOrId)
//	{
//		myXPosition = theXPosition;
//		myYPosition = theYPosition;
//		myMarkerType = theMarkerType;
//		myColor = theColor;
//		myIndex = theScaleOrId;
//	}

//	Sample2D_Markers::Sample2D_Markers(const double theXPosition, const double theYPosition,
//		const opencascade::handle<Graphic3d_ArrayOfPoints>& theArrayOfPoints, const Aspect_TypeOfMarker theMarkerType,
//		const Quantity_Color theColor, const double theScaleOrId)
//	{
//		myXPosition = theXPosition;
//		myYPosition = theYPosition;
//		myMarkerType = theMarkerType;
//		myColor = theColor;
//		myIndex = theScaleOrId;
//		myArrayOfPoints = theArrayOfPoints;
//	}


//#ifdef OCC7_5_0
//	void Sample2D_Markers::Compute(const Handle(PrsMgr_PresentationManager3d)&,
//		const Handle(Prs3d_Presentation)& aPresentation, const Standard_Integer)
//	{
//		if (myMarkerType == Aspect_TOM_USERDEFINED)
//		{
//			Handle(Graphic3d_AspectMarker3d) aMarker = new Graphic3d_AspectMarker3d(Aspect_TOM_POINT, myColor, myIndex);
//			Prs3d_Root::CurrentGroup(aPresentation)->SetGroupPrimitivesAspect(aMarker);
//			Prs3d_Root::CurrentGroup(aPresentation)->AddPrimitiveArray(myArrayOfPoints);
//		}
//		else
//		{
//			Handle(Graphic3d_AspectMarker3d) aMarker = new Graphic3d_AspectMarker3d(myMarkerType, myColor, myIndex);
//			Prs3d_Root::CurrentGroup(aPresentation)->SetPrimitivesAspect(aMarker);
//			Handle(Graphic3d_ArrayOfPoints) anArrayOfPoints = new Graphic3d_ArrayOfPoints(1);
//			anArrayOfPoints->AddVertex(myXPosition, myYPosition, 0);
//			Prs3d_Root::CurrentGroup(aPresentation)->AddPrimitiveArray(anArrayOfPoints);
//		}
//	}
//#elif defined(OCC7_9_0)
//	void Sample2D_Markers::Compute(const Handle(PrsMgr_PresentationManager)&,
//		const Handle(Prs3d_Presentation)& aPresentation,
//		const Standard_Integer)
//	{
		// 1. 获取当前 Group
//		Handle(Graphic3d_Group) aGroup = aPresentation->CurrentGroup();

		// 2. 创建统一的属性对象 (替代 Graphic3d_AspectMarker3d)
//		Handle(Graphic3d_Aspects) anAspect = new Graphic3d_Aspects();

//		if (myMarkerType == Aspect_TOM_USERDEFINED)
//		{
			// 设置为点模式
//			anAspect->SetMarkerType(Aspect_TOM_POINT);
			// 注意：OCCT 7.9 不再支持通过整数索引 (myIndex) 来指定自定义贴图
			// 如果 myIndex 仅仅是缩放比例，就这样写：
//			anAspect->SetMarkerScale(myIndex);
//			anAspect->SetColor(myColor);

			// 应用属性
//			aGroup->SetPrimitivesAspect(anAspect);
//			aGroup->AddPrimitiveArray(myArrayOfPoints);
//		}
//		else
//		{
			// 设置标准 Marker 类型
//			anAspect->SetMarkerType(myMarkerType);
//			anAspect->SetColor(myColor);
//			anAspect->SetMarkerScale(myIndex); // 假设 myIndex 在旧代码中对应 Scale 参数

			// 应用属性
//			aGroup->SetPrimitivesAspect(anAspect);

			// 创建点阵列
//			Handle(Graphic3d_ArrayOfPoints) anArrayOfPoints = new Graphic3d_ArrayOfPoints(1);
//			anArrayOfPoints->AddVertex(myXPosition, myYPosition, 0);

//			aGroup->AddPrimitiveArray(anArrayOfPoints);
//		}
//	}
//#endif 
//}
