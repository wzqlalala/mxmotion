#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "miofile_global.h"

//Qt
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDataStream>

namespace MechIOFile
{
	/*
	* .mxses 会话文件内存结构（纯数据，无 GUI 依赖）。
	* 文件容器与 mxcae 一致：quint32 magic + quint32 version + 分块 qCompress。
	* 解压后的 QDataStream 依次写入：视图树布局 -> 模型列表（含视口状态与模型数据chunk）。
	* 渲染视口引用一律使用视口名字符串（全局唯一），恢复时映射到新视口ID；
	* 图表卡片（曲线模块，非渲染视口）按曲线文档图表节点id引用。
	*/

	//单个视口数据（名称、类型与用户重命名标记；显示状态/相机等不保存。
	//视口名全局唯一（renameViewport 强制），页结构按视口名单键引用）
	struct MxsesViewportState
	{
		QString name;
		int viewportType = 0;        //MxFunctions::ViewportType
		bool userRenamed = false;    //用户手动改名标记（影响模型重命名时的名称联动）
	};

	//页内条目（V4）：渲染视口或曲线图表卡片，按页内节点序交错排列。
	//视口按视口名（全局唯一）引用；卡片按曲线文档图表节点id引用
	//（图表本名"XY Plot"等不唯一，且卡片ID每次运行重新分配，均不可作持久化键）
	struct MxsesPageItem
	{
		int kind = 0;        //0=渲染视口（按 name 引用） 1=图表卡片（按 refId 引用）
		QString name;        //视口名 / 卡片标题（显示与登记名）
		int refId = -1;      //kind=1 时为曲线文档图表节点id
	};

	//单个模型条目
	struct MxsesModelEntry
	{
		QString modelName;
		bool isPost = false;
		QString mxcaePath;
		QList<MxsesViewportState> viewports;
		QByteArray modelDataChunk;   //QDataStream << *mModelData 的产物
		QStringList resultFiles;     //V4：后处理模型的结果文件路径（恢复时重导入）
		QByteArray postItemsChunk;   //V4：后处理树条目（ItemInfo 摘要）序列化块
	};

	//视图树页条目（视口按名称记录；视口名全局唯一）
	struct MxsesPageEntry
	{
		QString name;
		int layoutIndex = 0;
		QStringList viewportOrder;       //页内视口节点序（视口名）——V3 兼容字段（读入时填）
		QStringList visibleViewports;    //手动布局可见组（可空）——V3 兼容字段（读入时填）
		QList<MxsesPageItem> items;      //V4：页内条目（视口+卡片，按节点序交错）
		QList<MxsesPageItem> visibleItems;  //V4：手动布局可见组（与 items 同构）
	};

	//整个会话文件
	struct MxsesFileData
	{
		QList<MxsesPageEntry> pages;
		QString currentPageName;
		QString activeViewportName;      //保存时的激活视口名（空表示无）
		bool viewportSynced = false;
		QList<MxsesModelEntry> models;
		QByteArray curveDocChunk;        //V4：曲线文档整树快照（CurveDocSerialize）
		QStringList curveSourceFiles;    //V4：曲线 pch 数据集路径（按登记序，恢复时重解析）
		int curveCurrentNodeId = -1;    //V4：保存时曲线树选中节点
		int activeChartNodeId = -1;     //V4：保存时激活视口为图表卡片时的曲线文档图表
		                                 //节点id（-1=非卡片激活）。卡片不在渲染视口注册表，
		                                 //卡片激活时 activeViewportName 为空，此字段与其
		                                 //互补承载保存时的曲线上下文；卡片ID每次运行重新
		                                 //分配，节点id随 curveDocChunk 稳定，故按节点id引用
	};

	/*
	* 读入文件的版本号（仿 modelData 的 FILE_VERSION 机制）：
	* 读端（mIMxsesFile）解析文件头后赋值，operator>> 据此分发到对应版本的
	* InStream 函数；写端固定写当前版本
	*/
	extern MIOFILE_EXPORT quint32 MXSES_FILE_VERSION;

	/*
	* V4 起的字段存在位标志（读端解析 MxsesFileData 头部后赋值，条目级
	* InStreamV4 按位分发条件字段；写端按写出的字段置位）。
	* 二期新增字段（每视口后处理GUI状态、测量历史）占新位并追加在流末尾，
	* 一期程序读二期文件时跳过未知位字段不受影响（容器整体解压后多余尾部
	* 字节天然被忽略），二期程序读一期文件按缺位走默认值
	*/
	extern MIOFILE_EXPORT quint32 MXSES_FIELD_MASK;

	namespace MxsesFieldMask
	{
		enum Bits : quint32
		{
			CurveDoc = 0x1,        //曲线文档块（curveDocChunk/curveSourceFiles/curveCurrentNodeId）
			ViewportGuiData = 0x2, //预留（二期）：每视口后处理GUI状态 guiDataChunk
			MeasureHistory = 0x4,  //预留（二期）：后处理测量历史 measureChunk
			ActiveChartCard = 0x8, //保存时激活视口为图表卡片（activeChartNodeId）
		};
	}

	/*
	* 版本化读函数（仿 modelData 仓库的 InStreammModelDataV3 模式）：
	* 读端 operator>> 按文件版本号（MXSES_FILE_VERSION）分发到对应 InStream 函数；
	* 写端 operator<< 直接按当前格式写出，不做版本封装。
	* V3(0x3000)：前处理模型/渲染视口/页结构（旧字段）；读入时由旧名列表合成 items。
	* V4(0x4000)：+ 页内图表卡片条目、曲线文档快照、后处理结果文件与树条目。
	* 后续格式演进时新增 V5 函数并在此分发
	*/
	QDataStream& InStreamMxsesViewportStateV3(QDataStream& in, MxsesViewportState& state);
	QDataStream& InStreamMxsesModelEntryV3(QDataStream& in, MxsesModelEntry& entry);
	QDataStream& InStreamMxsesPageEntryV3(QDataStream& in, MxsesPageEntry& entry);
	QDataStream& InStreamMxsesFileDataV3(QDataStream& in, MxsesFileData& data);
	QDataStream& InStreamMxsesModelEntryV4(QDataStream& in, MxsesModelEntry& entry);
	QDataStream& InStreamMxsesPageEntryV4(QDataStream& in, MxsesPageEntry& entry);
	QDataStream& InStreamMxsesFileDataV4(QDataStream& in, MxsesFileData& data);

	MIOFILE_EXPORT QDataStream& operator<<(QDataStream& out, const MxsesViewportState& state);
	MIOFILE_EXPORT QDataStream& operator>>(QDataStream& in, MxsesViewportState& state);

	MIOFILE_EXPORT QDataStream& operator<<(QDataStream& out, const MxsesPageItem& item);
	MIOFILE_EXPORT QDataStream& operator>>(QDataStream& in, MxsesPageItem& item);

	MIOFILE_EXPORT QDataStream& operator<<(QDataStream& out, const MxsesModelEntry& entry);
	MIOFILE_EXPORT QDataStream& operator>>(QDataStream& in, MxsesModelEntry& entry);

	MIOFILE_EXPORT QDataStream& operator<<(QDataStream& out, const MxsesPageEntry& entry);
	MIOFILE_EXPORT QDataStream& operator>>(QDataStream& in, MxsesPageEntry& entry);

	MIOFILE_EXPORT QDataStream& operator<<(QDataStream& out, const MxsesFileData& data);
	MIOFILE_EXPORT QDataStream& operator>>(QDataStream& in, MxsesFileData& data);
}
