#pragma once

namespace MechCommon
{
	/*
	* 读取文件的类型
	*/
	enum ReadFileMethod
	{
		//仅读取模型数据
		MeshDataOnly,

		//仅读取结果数据
		ResultDataOnly,

		//二者均读取
		BothData,
	};

	/*
	* 变量转换类型
	*/
	enum class QVariantType
	{
		QVariant_Int,

		QVariant_Double,
	};

	/*
	* 读取文件错误类型
	*/
	enum IOFileErrorType
	{
		//未知
		UnknownError,

		//文件不存在
		FileNotExist,

		//无法打开文件
		FileNotOpen,

		//文件版本不匹配
		FileVersionFalse,

		//数据库不存在
		DatabaseNull,

		//模型不匹配
		ModelMismatching,

		//当前结果不存在
		ResultTypeNotExist,

		//当前帧不存在
		FrameNotExist,
	};

	/*
	* 结果数据类型
	* 若改变枚举顺序，则需要相应改变mSourceData类中的数据标识容器
	*/
	enum ResultDataType
	{
		//节点位移
		NodeDisplacement,

		//Bar End Load
		BarEndLoad,

		//CBAR应力
		CbarStress,

		//CSHEAR应力
		CshearStress,

		//CSHEAR载荷
		CshearForce,

		//SHELL应变
		ShellStrain,

		//SHELL应力
		ShellStress,

		//SHELL Resultant载荷
		ShellRForce,

		//SHELL Moment载荷
		ShellMForce,

		//外部载荷
		AppliedForce,

		//SPC载荷
		SPCForce,

		//MPC载荷
		MPCForce,

		//CBAR载荷
		CbarForce,

		//CROD载荷
		CrodForce,

		//TRIA3载荷
		Tria3Force,

		//QUAD4载荷
		Quad4Force,

		//CBAR应变
		CbarStrain,

		//CROD应变
		CrodStrain,

		//TRIA3应变
		Tria3Strain,

		//QUAD4应变
		Quad4Strain,

		//CROD应力
		CrodStress,

		//TRIA3应力
		Tria3Stress,

		//QUAD4应力
		Quad4Stress,

		//节点平衡力
		NodeBalanceForce,
	};
}