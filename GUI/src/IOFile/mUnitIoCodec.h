#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "miofile_global.h"
#include "mUnitEnum.h"
#include "mPhysicalFieldEnum.h"
#include <QVector>

namespace CommonData
{
	class mUnitSystemManager;
}

namespace MechIOFile
{
	/*
	* .mx 文件单位制编解码器
	* 约定：.mx 文件本身不携带单位信息，文件中的数值一律按"当前界面单位制"解释；
	* 内存模型数据固定使用内部单位制 mm-t-N-s（上下文{3,0,0,0}）。
	* 读文件用 decode（当前单位制 → 内部），写文件用 encode（内部 → 当前单位制）。
	* 当前单位制与内部单位制一致（默认索引3）时所有倍数为1，行为与不转换完全相同。
	*/
	class MIOFILE_EXPORT mUnitIoCodec
	{
	public:
		/*
		* 内部单位制上下文{单位制,角度,转速,温度}，固定为 mm-t-N-s / 度 / rad/s / 摄氏
		*/
		static const QVector<int>& internalUnitsIndex();

		/*
		* 当前界面单位制上下文（取自当前模型的单位系统管理器）
		*/
		static QVector<int> currentUnitsIndex();

		/*
		* 读文件：文件值（按当前界面单位制解释）→ 内部值（mm-t-N-s）
		* 普通量纲（倍数换算）；温度绝对值请用 decodeTemp
		*/
		static double decode(const MxFunctions::Unit& unit, double fileValue);

		/*
		* 写文件：内部值（mm-t-N-s）→ 文件值（当前界面单位制）
		* 普通量纲（倍数换算）；温度绝对值请用 encodeTemp
		*/
		static double encode(const MxFunctions::Unit& unit, double internalValue);

		/*
		* 读文件：温度绝对值（仿射换算，含偏移，禁止倍数近似）
		*/
		static double decodeTemp(double fileValue);

		/*
		* 写文件：温度绝对值（仿射换算，含偏移，禁止倍数近似）
		*/
		static double encodeTemp(double internalValue);

		/*
		* 读文件：曲线/表格数据批量换算（原地）
		*/
		static void decodeCurve(const MxFunctions::Unit& unit, QVector<double>& values);

		/*
		* 写文件：曲线/表格数据批量换算（原地）
		*/
		static void encodeCurve(const MxFunctions::Unit& unit, QVector<double>& values);

		/*
		* 材料属性类型 → 物理量纲映射（用于材料标量/曲线收口点换算；
		* 未映射类型返回 NoneUnit，倍数为1不换算，与现状一致）
		*/
		static MxFunctions::Unit unitOfMaterialProp(MxFunctions::MaterialPropType proType);

	private:
		/*
		* 当前模型的单位系统管理器（可能为空，调用方须判空）
		*/
		static CommonData::mUnitSystemManager* unitManager();

		/*
		* 任意两上下文间的倍数换算
		* 覆盖 mUnitSystemManager::getUnitSystemFromToRatio 未分发的量纲
		*（热导率族走专用 FromTo，与材料面板提交路径保持一致；未覆盖量纲委托默认实现）
		*/
		static double ratio(const MxFunctions::Unit& unit, const QVector<int>& fromidx, const QVector<int>& toidx);

		/*
		* 温度仿射换算（单位串间，k*x+b）
		*/
		static double tempAffine(double value, const QString& fromUnit, const QString& toUnit);
	};
}
