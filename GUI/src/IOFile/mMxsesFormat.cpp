#include "mMxsesFormat.h"

namespace MechIOFile
{
	//读入文件的版本号（读端解析文件头后赋值，operator>>据此分发）
	quint32 MXSES_FILE_VERSION = 0;

	//V4 起的字段存在位标志（读端解析 MxsesFileData 头部后赋值）
	quint32 MXSES_FIELD_MASK = 0;

	/*************************** 版本化读函数 ***************************/

	QDataStream& InStreamMxsesViewportStateV3(QDataStream& in, MxsesViewportState& state)
	{
		qint32 type = 0;
		in >> state.name >> type >> state.userRenamed;
		state.viewportType = type;
		return in;
	}

	QDataStream& InStreamMxsesModelEntryV3(QDataStream& in, MxsesModelEntry& entry)
	{
		in >> entry.modelName >> entry.isPost >> entry.mxcaePath;
		in >> entry.viewports;       //逐项调 operator>>（含版本分发）
		in >> entry.modelDataChunk;
		return in;
	}

	QDataStream& InStreamMxsesPageEntryV3(QDataStream& in, MxsesPageEntry& entry)
	{
		qint32 layout = 0;
		in >> entry.name >> layout;
		entry.layoutIndex = layout;
		in >> entry.viewportOrder >> entry.visibleViewports;
		//V4 字段由旧名列表合成（全部为渲染视口：V3 时代页内无图表卡片）
		for (const QString& name : entry.viewportOrder)
		{
			MxsesPageItem item;
			item.kind = 0;
			item.name = name;
			entry.items.append(item);
		}
		for (const QString& name : entry.visibleViewports)
		{
			MxsesPageItem item;
			item.kind = 0;
			item.name = name;
			entry.visibleItems.append(item);
		}
		return in;
	}

	QDataStream& InStreamMxsesFileDataV3(QDataStream& in, MxsesFileData& data)
	{
		MXSES_FIELD_MASK = 0;   //V3 无字段位标志，清残留（防上次读V4文件留下的旧值）
		in >> data.pages;
		in >> data.currentPageName >> data.activeViewportName >> data.viewportSynced;
		in >> data.models;
		return in;
	}

	QDataStream& InStreamMxsesModelEntryV4(QDataStream& in, MxsesModelEntry& entry)
	{
		InStreamMxsesModelEntryV3(in, entry);
		in >> entry.resultFiles >> entry.postItemsChunk;
		//二期条件字段（ViewportGuiData/MeasureHistory 位）追加在此，按
		//MXSES_FIELD_MASK 分发，当前版本无对应位字段
		return in;
	}

	QDataStream& InStreamMxsesPageEntryV4(QDataStream& in, MxsesPageEntry& entry)
	{
		qint32 layout = 0;
		in >> entry.name >> layout;
		entry.layoutIndex = layout;
		in >> entry.items >> entry.visibleItems;
		return in;
	}

	QDataStream& InStreamMxsesFileDataV4(QDataStream& in, MxsesFileData& data)
	{
		//字段存在位标志先读：列表内条目的条件字段（二期）按此分发
		quint32 mask = 0;
		in >> mask;
		MXSES_FIELD_MASK = mask;
		in >> data.pages;
		in >> data.currentPageName >> data.activeViewportName >> data.viewportSynced;
		in >> data.models;
		if (mask & MxsesFieldMask::CurveDoc)
		{
			qint32 curNode = -1;
			in >> data.curveSourceFiles >> curNode >> data.curveDocChunk;
			data.curveCurrentNodeId = curNode;
		}
		if (mask & MxsesFieldMask::ActiveChartCard)
		{
			qint32 activeChart = -1;
			in >> activeChart;
			data.activeChartNodeId = activeChart;
		}
		return in;
	}

	/*************************** 流操作符 ***************************/
	//写端直接按当前格式（V4）写出；读端按文件版本分发到 InStream 函数

	QDataStream& operator<<(QDataStream& out, const MxsesViewportState& state)
	{
		out << state.name;
		out << qint32(state.viewportType);
		out << state.userRenamed;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, MxsesViewportState& state)
	{
		InStreamMxsesViewportStateV3(in, state);   //V3/V4 视口结构未变
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const MxsesPageItem& item)
	{
		out << qint32(item.kind) << item.name << qint32(item.refId);
		return out;
	}

	QDataStream& operator>>(QDataStream& in, MxsesPageItem& item)
	{
		qint32 kind = 0;
		qint32 refId = -1;
		in >> kind >> item.name >> refId;
		item.kind = kind;
		item.refId = refId;
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const MxsesModelEntry& entry)
	{
		out << entry.modelName << entry.isPost << entry.mxcaePath;
		out << entry.viewports;
		out << entry.modelDataChunk;
		out << entry.resultFiles << entry.postItemsChunk;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, MxsesModelEntry& entry)
	{
		if (MXSES_FILE_VERSION >= 0x4000)
		{
			InStreamMxsesModelEntryV4(in, entry);
		}
		else
		{
			InStreamMxsesModelEntryV3(in, entry);
		}
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const MxsesPageEntry& entry)
	{
		out << entry.name;
		out << qint32(entry.layoutIndex);
		out << entry.items << entry.visibleItems;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, MxsesPageEntry& entry)
	{
		if (MXSES_FILE_VERSION >= 0x4000)
		{
			InStreamMxsesPageEntryV4(in, entry);
		}
		else
		{
			InStreamMxsesPageEntryV3(in, entry);
		}
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const MxsesFileData& data)
	{
		//字段存在位标志先写（读端条目级条件字段按位分发；新字段占新位
		//追加在流末尾，保证旧程序读新文件时尾部多余字段被安全忽略）
		quint32 mask = MxsesFieldMask::CurveDoc | MxsesFieldMask::ActiveChartCard;
		out << mask;
		out << data.pages;
		out << data.currentPageName << data.activeViewportName << data.viewportSynced;
		out << data.models;
		out << data.curveSourceFiles << qint32(data.curveCurrentNodeId) << data.curveDocChunk;
		out << qint32(data.activeChartNodeId);
		return out;
	}

	QDataStream& operator>>(QDataStream& in, MxsesFileData& data)
	{
		if (MXSES_FILE_VERSION >= 0x4000)
		{
			InStreamMxsesFileDataV4(in, data);
		}
		else
		{
			InStreamMxsesFileDataV3(in, data);
		}
		return in;
	}
}
