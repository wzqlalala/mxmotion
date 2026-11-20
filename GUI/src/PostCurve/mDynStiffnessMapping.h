#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 动刚度/VTF 曲线映射配置（CSV 模板）核心（纯逻辑，无 UI）
//
// 设计稿 01（NVH曲线-动刚度曲线）与 VTF 曲线设计稿两步流程的数据中枢：
//   1. 模板配置（mTemplateConfigDlg）：导入 pch 结果文件，按所选模板类型
//      （动刚度曲线/VTF曲线）与形式（位移/速度/加速度）提取全部数据块，
//      落成 CSV 映射文件（目标值列为默认参数，允许用户手工编辑再保存）；
//   2. 动刚度曲线（mDynStiffnessDlg）/ VTF 曲线（mVtfCurveDlg）：导入
//      pch + 上一步的 CSV，形式从 CSV 头部元信息自动识别，逐行建
//      实测曲线 + 目标值曲线。
//
// CSV 格式（UTF-8 带 BOM，逗号分隔，含引号转义）：
//   #Template=动刚度曲线        ← 元信息行（# 开头，读取时识别模板类型）
//   #Form=位移                  ← 形式（动刚度：位移/速度/加速度；VTF：速度/加速度；
//                                  缺失时动刚度按首行 Y轴内容推断、VTF 回退速度）
//   动刚度（6 列）：
//   工况（SUBCASE）,结果类型（Types）,对象（Requests）,分量（Components）,目标值（Target）,Y轴内容
//   Subcase 101 (label),DISPLACEMENTS,Point id 1001,MAG|X-Trans,10000N/mm,Kd/N/m
//   VTF（5 列，无 Y轴内容列）：
//   工况（SUBCASE）,结果类型（Types）,对象（Requests）,分量（Components）,目标值（Target）
//   Subcase 1 (label),VELOCITY,Point id 6001,MAG|X-Trans,0.1 mm/s
//
// 分量令牌与 mCurveDataSourceStore::expandedComponents/componentIndex 同一
// 套命名（"MAG|X-Trans" 等），可直接传 assembleChannelData 装配通道数据。
// 结果类型文本即 PchReader::toString(resultType)（"DISPLACEMENTS"/"VELOCITY"/
// "ACCELERATION"），与 pch 数据块的 schemaName 同源。
// =============================================================================

#include "PostCurveGlobal.h"
#include "PchReader/PchReader.h"

#include <QString>
#include <QList>

namespace PostCurve
{
	//动刚度形式（动刚度：位移默认；VTF：速度默认；NTF：声压默认。
	//SoundPressure/SoundPressureIpi 为 NTF 专用形式）
	enum class DynStiffForm
	{
		Displacement = 0,   //位移
		Velocity,           //速度
		Acceleration,       //加速度
		SoundPressure,      //声压（NTF）
		SoundPressureIpi,   //声压+激励点IPI（NTF：声压行 + 激励点加速度行）
	};

	//模板类型（模板配置对话框"模板选择"下拉的三项）
	enum class CurveTemplateKind
	{
		DynStiffness = 0,   //动刚度曲线
		Vtf,                //VTF曲线
		Ntf,                //NTF曲线
	};

	//形式中文文本（位移/速度/加速度/声压/声压+激励点IPI）
	POSTCURVE_EXPORT QString dynStiffFormText(DynStiffForm form);
	//形式 -> 结果类型（位移→Displacements / 速度→Velocity / 加速度→Acceleration /
	//声压→Pressure；声压+激励点IPI 含两类结果块，不适用本映射——
	//buildRowsFromPch 与创建侧按行 resultTypeText 分派）
	POSTCURVE_EXPORT PchReader::ResultType dynStiffFormResultType(DynStiffForm form);
	//结果类型文本 -> 枚举（"DISPLACEMENTS"/"VELOCITY"/"ACCELERATION"/"PRESSURE"，
	//即 CSV 结果类型列文本；未识别返回 Unknown）
	POSTCURVE_EXPORT PchReader::ResultType resultTypeFromText(const QString& typeText);
	//模板类型中文文本（动刚度曲线/VTF曲线/NTF曲线，即 CSV #Template 元信息值）
	POSTCURVE_EXPORT QString curveTemplateKindText(CurveTemplateKind kind);
	//目标值默认参数：
	//导出模板（buildRowsFromPch）的目标值默认取该行数据 Y 值区间的中点，
	//值空间/单位随导出类型（kind+form）：
	//VTF：目标=常数横线（值空间=Y 本身）→ 速度 mm/s、加速度 mm/s^2；
	//动刚度：CSV 存刚度 K（N/mm）——位移形式目标线画在 kd=1/|H| 空间（中点取
	//kd 区间）、速度/加速度形式目标曲线为 2πf/K、(2πf)²/K（K 按频率/幅值区间
	//中点反解）；幅值装配失败回退固定默认文本；
	//解析 CSV（readMappingFile）与创建兜底仍用固定默认——
	//动刚度 10000N/mm、VTF 速度=0.1 mm/s、加速度=100 mm/s^2；
	//NTF 无固定默认（声压行目标值提取自 pch $PRESSURE 标签的 dB 值，
	//激励点 IPI 行不生成目标线）——返回空串
	POSTCURVE_EXPORT QString dynStiffDefaultTargetText();
	POSTCURVE_EXPORT QString curveDefaultTargetText(CurveTemplateKind kind, DynStiffForm form);
	//Y轴内容默认参数（仅动刚度 CSV 有该列；VTF 无）：
	//位移=Kd/N/m，速度=Velocity Mobility (m/s)/N，加速度=IPI((m/s^2)/N
	POSTCURVE_EXPORT QString dynStiffDefaultYAxisText(DynStiffForm form);

	//CSV 映射文件的一行（一条待生成曲线的通道 + 手工参数；VTF 场景 yAxisText 恒空）
	struct POSTCURVE_EXPORT DynStiffRow
	{
		int subcaseId = 0;
		QString subcaseText;      //工况（"Subcase 101 (label)"，subcaseDisplayText 同源）
		QString resultTypeText;   //结果类型（"DISPLACEMENTS" 等，= 块 schemaName）
		int pointId = 0;
		QString objectText;       //对象（"Point id 1001"）
		QString componentText;    //分量令牌（"MAG|X-Trans"）
		QString targetText;       //目标值（默认参数兜底）
		QString yAxisText;        //Y轴内容（动刚度专用，VTF 恒空）
	};

	//从 pch 提取指定形式的模板行：遍历匹配 ResultType 的数据块，每个
	//工况×对象 出 X/Y/Z 三个平动分量行（旋转分量按设计稿不导出；
	//用户可在 CSV 中手动删改）。
	//NTF 特例：
	//  - 声压：遍历 Pressure 块，每 工况×对象 一行，分量令牌 "REL|FX"
	//    （设计稿令牌，REAL 声压 × FX 激励方向），目标值提取自块标签
	//    $PRESSURE 的 dB 值（如 "PRESSURE (58 dB)" → "58dB"；无则空）
	//  - 声压+激励点IPI：声压行 + Acceleration 块的 X/Y/Z 行（MAG|X-Trans…，
	//    目标值空=不生成目标线，对齐设计稿 IPI 图无目标线）
	//行序随 blocks 顺序（稳定，重复调用结果一致）
	//模板类型/形式 -> 提取该形式全部数据行（导出模板用）：
	//动刚度/VTF 走矢量三分量通用路径（每 工况×对象×平动分量 一行），
	//NTF 走声压/激励点 IPI 特例分支
	POSTCURVE_EXPORT QList<DynStiffRow> buildRowsFromPch(const PchReader::PchFile& file,
		CurveTemplateKind kind, DynStiffForm form);

	//写 CSV 映射文件（UTF-8 带 BOM；#Template/#Form 元信息行 + 表头 + 数据行；
	//动刚度/NTF 6 列（NTF 的 Y轴内容列恒空）、VTF 5 列；rows 里的空 targetText
	//按模板默认参数补齐——NTF 无固定默认，空保持空=不生成目标线）
	POSTCURVE_EXPORT bool writeMappingFile(const QString& filePath, CurveTemplateKind kind,
		DynStiffForm form, const QList<DynStiffRow>& rows, QString* err = nullptr);

	//读 CSV 映射文件（容忍用户手工编辑：元信息行可缺失、列内空白容忍；
	//模板类型按 #Template 元信息识别，缺失时按首数据行列数推断——5 列=VTF、
	//6 列=动刚度（NTF 与动刚度同列数，依赖 #Template 区分）。
	//空 targetText/yAxisText 按模板默认参数补齐（NTF 保持空）。
	//失败时 err 带原因，rows 必空）
	POSTCURVE_EXPORT bool readMappingFile(const QString& filePath, CurveTemplateKind& kind,
		DynStiffForm& form, QList<DynStiffRow>& rows, QString* err = nullptr);

	//目标值数值解析："10000N/mm" -> 10000、"0.1 mm/s" -> 0.1（提取前导浮点数；
	//纯文本无数字返回 false）
	POSTCURVE_EXPORT bool parseTargetValue(const QString& targetText, double& value);
}
