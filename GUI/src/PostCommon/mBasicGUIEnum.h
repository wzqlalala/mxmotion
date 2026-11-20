#pragma once

namespace PostCommon
{
	//数据类型枚举
	enum DATATYPE
	{
		//部件数据
		Data_Part,

		//实例数据
		Data_Instance,

		//材料数据
		Data_Material,

		//截面数据
		Data_Section,

		//截面指派数据
		Data_SectionAssignment,

		//剖面数据
		Data_Profile,

		//分析步数据
		Data_Step,

		//场输出数据
		Data_FOutput,

		//历程输出数据
		Data_HOutput,

		//约束数据
		Data_Constraint,

		//载荷数据
		Data_Load,

		//边界数据
		Data_BC,

		//预定义场数据
		Data_Predefined,

		//作业数据
		Data_Job,

		//输出数据库数据
		Data_OPDatabase,
		
		//模型数据库数据
		Data_ModelDatabase,
		
		//XY视图数据
		Data_XYPlot,
		
		//XY数据数据
		Data_XYData,

		//装配-坐标系数据
		Data_AssemblyDatum,

		//部件-坐标系数据
		Data_PartDatum,

		//装配-弹簧阻尼数据
		Data_AssemblySpringDashpots,

		//部件-弹簧阻尼数据
		Data_PartSpringDashpots,

		//幅值数据
		Data_Amplitude,

		//装配-惯性数据
		Data_AssemblyInertia,

		//部件-惯性数据
		Data_PartInertia,

		//装配-集合数据
		Data_AssemblySet,

		//装配-实例数据
		Data_AssemblyInstance,

		//部件-集合数据
		Data_PartSet,

		//部件-内部集合数据
		Data_InternalPartSet,

		//后处理云图数据
		Data_PostCloud,

		//后处理动画图数据
		Data_PostAnimation,

		//后处理曲线视图数据
		Data_PostXYView,

		//后处理曲线数据
		Data_PostXYData,

		//后处理通用数据
		Data_PostCommon,

		//后处理符号绘制数据
		Data_PostSymbol,

		//后处理重叠数据
		Data_PostSuperimpose,

		//后处理结果数据
		Data_PostResult,

		//后处理ODB数据
		Data_PostODB,

		//后处理材料方向数据
		Data_PostMaterialOrientation,

	    //后处理铺层叠放数据
		Data_PostPlyStack,
	};

	/*
	* 功能模块
	*/
	enum FUNCTIONMODULE
	{
		//Part
		Module_Part,

		//Property
		Module_Property,

		//Assembly
		Module_Assembly,

		//Step
		Module_Step,

		//Interaction
		Module_Interaction,

		//Load
		Module_Load,

		//Mesh
		Module_Mesh,

		//Job
		Module_Job,

		//Visualization
		Module_Visual,

	};

	/*
	* 模型树类型
	*/
	enum TREETYPE
	{
		//Model根节点
		Item_ModelRoot,

		//Model子节点
		Item_ModelChild,

		//部件父节点
		Item_PartParent,

		//部件子节点
		Item_PartChild,

		//部件-features
		Item_Part_features,

		//部件-sets
		Item_Part_sets,

		//部件-surfaces
		Item_Part_surfaces,

		//部件-skins
		Item_Part_skins,

		//部件-stringers
		Item_Part_stringers,

		//部件-section assignments
		Item_Part_section,

		//部件-orientations
		Item_Part_orientations,

		//部件-composite layups
		Item_Part_composite,

		//部件-engineering features
		Item_Part_engineering,

		//部件-inertials
		Item_Part_inertials,

		//部件-springs/dashpots
		Item_Part_springs,

		//部件-mesh
		Item_Part_mesh,

		//材料父节点
		Item_MaterialParent,

		//材料子节点
		Item_MaterialChild,

		//截面父节点
		Item_SectionParent,

		//截面子节点
		Item_SectionChild,

		//剖面父节点
		Item_ProfileParent,

		//剖面子节点
		Item_ProfileChild,

		//装配节点
		Item_Assembly,

		//装配-实例父节点
		Item_InstanceParent,

		//装配-实例子节点
		Item_InstanceChild,

		//装配-特征父节点
		Item_FeatureParent,

		//装配-特征子节点
		Item_FeatureChild,

		//装配-集合父节点
		Item_AssemblySetParent,

		//装配-集合子节点
		Item_AssemblySetChild,

		//装配-表面父节点
		Item_AssemblySurfParent,

		//装配-表面子节点
		Item_AssemblySurfChild,

		//装配-工程特征节点
		Item_AssemblyEngineer,

		//装配-惯性父节点
		Item_AssemblyInertiasParent,

		//装配-惯性子节点
		Item_AssemblyInertiasChild,

		//装配-弹簧阻尼父节点
		Item_AssemblySpringParent,

		//装配-弹簧阻尼子节点
		Item_AssemblySpringChild,

		//分析步父节点
		Item_StepParent,

		//分析步子节点（初始分析步）
		Item_StepChildInit,

		//initBCs
		Item_StepInitBCParent,

		//initPred
		Item_StepInitPredParent,

		//分析步子节点（非初始分析步）
		Item_StepChild,

		//分析步-场输出父节点
		Item_StepFOPParent,

		//分析步-场输出子节点
		Item_StepFOPChild,

		//分析步-历程输出父节点
		Item_StepHOPParent,

		//分析步-历程输出子节点
		Item_StepHOPChild,

		//分析步-相互作用父节点
		Item_StepIntParent,

		//分析步-相互作用子节点
		Item_StepIntChild,

		//分析步-载荷父节点
		Item_StepLoadParent,

		//分析步-载荷子节点
		Item_StepLoadChild,

		//分析步-边界条件父节点
		Item_StepBCParent,

		//分析步-边界条件子节点
		Item_StepBCChild,

		//分析步-预定义场父节点
		Item_StepPredParent,

		//分析步-预定义场子节点
		Item_StepPredChild,

		//场输出父节点
		Item_FOutputParent,

		//场输出子节点
		Item_FoutputChild,

		//历程输出父节点
		Item_HOutputParent,

		//历程输出子节点
		Item_HoutputChild,

		//约束父节点
		Item_ConstraintParent,

		//约束子节点
		Item_ConstraintChild,

		//幅值父节点
		Item_AmplitudeParent,

		//幅值子节点
		Item_AmplitudeChild,

		//载荷父节点
		Item_LoadParent,

		//载荷子节点
		Item_LoadChild,

		//边界条件父节点
		Item_BoundaryParent,

		//边界条件子节点
		Item_BoundaryChild,

		//预定义场父节点
		Item_PredefinedParent,

		//预定义场子节点
		Item_PredefinedChild,

		//Analysis根节点
		ItemAnalysisRoot,

		//Job父节点
		ItemJobParent,

		//Job子节点
		ItemJobChild,

		//Output Databases
		Item_OPDatabaseParent,
		Item_OPDatabaseChild,
		Item_OPHOParent,
		Item_OPStepParent,
		Item_OPInstanceParent,
		Item_OPMaterialParent,
		Item_OPSectionParent,
		Item_OPElementParent,
		Item_OPNodeParent,
		Item_OPSurfaceParent,
		Item_OPSessionParent,
		Item_OPODBParent,
		Item_OPUsrDataParent,
		Item_OPAnnotationParent,
		Item_OPXYDataParent,
		Item_OPXYDataChild,
		

		//Model Databases
		Item_ModelDatabaseParent,
		Item_ModelDatabaseChild,
		Item_MDStepParent,
		Item_MDStepinitChild,
		Item_MDStepChild,
		Item_MDInstanceParent,
		Item_MDInstanceChild,

		//XYPlots
		Item_XYPlotParent,
		Item_XYPlotChild,

		//XYData
		Item_XYDataParent,
		Item_XYDataChild
	};

	/*
	* 单元类型
	*/
	enum ELMENTTYPE
	{
		//无
		Element_None = 0,

		//质量单元
		Element_MASS,

		//转动惯量单元
		Element_ROTARYI,

		//2节点线性3D桁架单元
		Element_T3D2,

		//2节点空间线性梁单元
		Element_B31,

		//连接器单元
		Element_CONN3D2,

		//阻尼器单元
		Element_DashpotA,

		//弹簧单元
		Element_SpringA,

		//3节点3D刚性三角形单元   （刚性部件不能指派截面）
		Element_R3D3,

		//4节点3D双线性刚性四边形单元
		Element_R3D4,

		//3节点三角形壳单元，有限膜应变
		Element_S3,

		//4节点曲面壳单元，有限膜应变
		Element_S4,

		//4节点曲面壳单元，减缩积分，沙漏控制，有限膜应变
		Element_S4R,

		//4节点线性四面体单元
		Element_C3D4,

		//10节点二次四面体单元
		Element_C3D10,

		//10节点修正二次四面体单元
		Element_C3D10M,

		//6节点线性三棱柱单元
		Element_C3D6,

		//8节点线性六面体单元
		Element_C3D8,

		//8节点线性六面体单元，减缩积分，沙漏控制
		Element_C3D8R,

		//8节点线性六面体单元，非协调模式
		Element_C3D8I,
	};



}