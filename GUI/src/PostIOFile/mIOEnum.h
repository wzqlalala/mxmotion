#pragma once

namespace MIOFile
{
	/** 读取文件的类型 */
	enum ReadFileMethod
	{
		MeshDataOnly = 0x0001,					//仅读取模型数据
		ResultDataOnly = 0x0002,				//仅读取结果数据
		BothData = 0x0004,						//二者均读取
	};

	/** 读取文件错误类型 */
	enum IOFileErrorType
	{
		UnknownError = 0x0001,					//未知
		FileNotExist = 0x0002,					//文件不存在
		FileNotOpen = 0x0004,					//无法打开文件
		FileVersionFalse = 0x0008,				//文件版本不匹配
		DatabaseNull = 0x0010,					//数据库不存在
		ModelMismatching = 0x0020,				//模型不匹配
		ResultTypeNotExist = 0x0040,			//当前结果不存在
		FrameNotExist = 0x0080,					//当前帧不存在
		Op2FileNotExist = 0x0100,				//OP2文件不存在
		Op2BdfNotFound = 0x0200,				//未找到OP2配套BDF文件
		Op2ParseFailed = 0x0400,				//OP2或BDF解析失败
		Op2UnsupportedTable = 0x0800,			//OP2结果表不受支持
		Op2ModelMismatch = 0x1000,				//OP2结果与BDF模型不匹配
		AltairConvertFailed = 0x2000,			//H3D转换失败（缺HyperWorks/许可证/导出脚本失败）
		AltairCompanionNotFound = 0x4000,		//未找到配套的.fem/.hwascii文件
		AltairParseFailed = 0x8000,				//.hwascii或.fem解析失败
		AltairUnsupportedData = 0x10000,		//无受支持的Altair结果数据
		AltairModelMismatch = 0x20000,			//Altair结果与网格模型不匹配
		NativeFileNotExist = 0x40000,			//原生读取（OP2/RST·RTH/d3plot/H3D/ODB）结果文件不存在
		NativeParseFailed = 0x80000,			//原生读取解析失败（库抛出ReadError）
		NativeUnsupportedLayout = 0x100000,		//文件格式布局不受支持或无有效结果数据
	};


	/** 结果数据类型 */
	/** 若改变枚举顺序，则需要相应改变mSourceData类中的数据标识容器 */
	enum ResultDataType
	{
		NodeDisplacement = 0x00000001,				//节点位移
		BarEndLoad = 0x00000002,					//Bar End Load
		CbarStress = 0x00000004,					//CBAR应力
		CshearStress = 0x00000008,					//CSHEAR应力
		CshearForce = 0x00000010,					//CSHEAR载荷
		ShellStrain = 0x00000020,					//SHELL应变
		ShellStress = 0x00000040,					//SHELL应力
		ShellRForce = 0x00000080,					//SHELL Resultant载荷
		ShellMForce = 0x00000100,					//SHELL Moment载荷
		AppliedForce = 0x00000200,					//外部载荷
		SPCForce = 0x00000400,						//SPC载荷
		MPCForce = 0x00000800,						//MPC载荷
		CbarForce = 0x00001000,						//CBAR载荷
		CrodForce = 0x00002000,						//CROD载荷
		Tria3Force = 0x00004000,					//TRIA3载荷
		Quad4Force = 0x00008000,					//QUAD4载荷
		CbarStrain = 0x00010000,					//CBAR应变
		CrodStrain = 0x00020000,					//CROD应变
		Tria3Strain = 0x00040000,					//TRIA3应变
		Quad4Strain = 0x00080000,					//QUAD4应变
		CrodStress = 0x00100000,					//CROD应力
		Tria3Stress = 0x00200000,					//TRIA3应力
		Quad4Stress = 0x00400000,					//QUAD4应力
		NodeBalanceForce = 0x00800000,				//节点平衡力
	};

}
