#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QString>
#include <QList>

#include "mAnalysisField.h"
#include "mBasicEnum.h"

using MechDiscreteFieldData::AnalysisFieldRow;

namespace MechMainWindow
{
	/*
	* 解析分析场文本数据文件（导入/文本文档转内联共用同一实现）。
	* 分隔符为空格或英文逗号，连续分隔符合并、忽略空 token；
	* 取 [startRow, endRow] 闭区间（1 起始；结束行小于起始行或超界时按实际可读范围处理，不报错）；
	* 丢弃 startColumn-1 之前的列；列数不足最少列数的行丢弃；分量不足补空、超出截断。
	* 最少列数：点云=3（X,Y,Z,分量…）；单元面=2（单元编号,面编号,分量…）；节点/单元=1（编号,分量…）。
	* 解析失败（文件不存在/打不开）时通过 error 返回提示。
	*/
	MMAINWINDOW_EXPORT QList<AnalysisFieldRow> parseAnalysisFieldTextFile(
		const QString& path, int startRow, int endRow, int startColumn,
		bool isPointCloud, MxFunctions::PickDataType locate, int componentCount,
		QString& error);

	/*
	* 导出分析场数据到文本文件：首行表头 + 数据行，逗号分隔。
	* 表头：点云 X,Y,Z,Value1,…,ValueN；节点/单元 ID,Value1,…；单元面 EID,FaceID,Value1,…。
	*/
	MMAINWINDOW_EXPORT bool exportAnalysisFieldTextFile(
		const QString& path, const QList<AnalysisFieldRow>& rows,
		bool isPointCloud, MxFunctions::PickDataType locate, int componentCount,
		QString& error);
}
