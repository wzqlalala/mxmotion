#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 数据源仓库（会话内已导入数据集的共享缓存，单例）
//
// "从文件创建曲线"（mOpenCurveDlg）与"定义曲线"（mDefineCurveDlg，设计图10~13）
// 共用同一份数据集：解析一次、处处可选，避免各弹窗各自持有解析缓存产生分叉。
// 原解析缓存是 mOpenCurveDlg 的私有成员（_FileData/_listWigetData），本类将其
// 抽出为模块级共享（字段与建索引逻辑原样迁移，原对话框行为不变）。
//
// 内容：
//   - 原始解析数据（按文件类型二分，fileKind 分发）：
//       .pch      -> PchReader::PchFile（blocks/records）
//       .csv/.txt -> mCurveAsciiReader::AsciiColumnFile（具名等长数值列，R22）
//     两类分存（_files/_asciiFiles），file() 仅返回 pch 数据集——不伪造 pch 块。
//   - 层级索引 ListWgtStruct：工况 -> 结果类型 -> 对象 -> 分量数值格式，
//     及 X 轴类型集合（供"从文件创建曲线"四级联动列表与定义曲线数据集树共用）。
//     ascii 数据集为固定骨架（见 buildIndex）：无横坐标类型，Unknown/Block 1
//     单块，分量列表经 asciiColumnNames 下发（不走 expandedComponents）。
//   - 分量名规则（自 mOpenCurveDlg 私有逻辑迁为共享）：
//       isVectorSchema     结果类型是否节点矢量（仅矢量结果有分量展开）
//       expandedComponents 数值格式 -> 12/6 个分量名
//       componentIndex     分量名 -> primary/secondary 数据下标
//
// 信号 sig_fileAdded：新数据集登记完成（解析+索引就绪），订阅方据此重建界面。
// =============================================================================

#include "PostCurveGlobal.h"
#include "PchReader/PchReader.h"
#include "mCurveAsciiReader.h"

//QT
#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QThread>
#include <QVector>

//STL
#include <atomic>
#include <memory>
#include <set>

namespace PostCurve
{
	//分量数值格式信息（ycomponent 索引叶子）：数值格式名 + 该数据块是否瞬态。
	//瞬态为块级判定（见 buildIndex），数据形态优先：频响复数每点 12 值必有
	//secondary，瞬态实数每点 6 值 secondary 必空；label 显式声明时间
	//（IDENTIFIED BY TIME）时直接认定。旧实现按文件级 label 文本判定，
	//无该标签的瞬态 deck（如 OptiStruct "$REAL OUTPUT"）会被误判为频响，
	//X 轴认成 Frequency 且分量误展开出 12 个（虚部/相位全为填 0 的假数据）
	struct NumericFmtInfo
	{
		QString fmt;               //数值格式名（PchReader::toString(numericFormat)，如 "REAL-ONLY"）
		bool transient = false;    //块是否瞬态（分量展开 6/12 与 X 轴类型按此分流）
		//std::set 按格式名排序/去重（同一对象同一格式至多一条，语义同旧 QString 集合）
		bool operator<(const NumericFmtInfo& other) const { return fmt < other.fmt; }
	};

	//数据集层级索引（自 mOpenCurveDlg 的同名结构迁移，字段语义不变；
	//ycomponent 叶子升级为携带瞬态标志的 NumericFmtInfo）：
	//X结果类型集合；Y侧 工况 -> 结果类型 -> 对象 -> 分量数值格式
	struct ListWgtStruct
	{
		//X
		std::set<QString> xresulttype;                //X结果类型（Time[Sec]/Frequency[Hz]）
		//Y
		std::set<int> operatingcondition;
		QHash<int, std::set<QString>> yresulttype;
		QHash<int, QHash<QString, std::set<int>>> object;
		QHash<int, QHash<QString, QHash<int, std::set<NumericFmtInfo>>>> ycomponent;
	};

	//装配/探针的块键（工况,结果类型,对象）——块索引 _blockIndexes 的键
	//（optimize-channel-map-perf）：schema 以显示串（blk.schemaName 的 QString
	//形态）参与匹配，与 assembleChannelData 现行匹配语义一致
	struct ChannelBlockKey
	{
		int subcaseId = 0;
		QString schema;
		int pointId = 0;
		bool operator==(const ChannelBlockKey& other) const
		{
			return subcaseId == other.subcaseId && schema == other.schema
				&& pointId == other.pointId;
		}
	};

	inline uint qHash(const ChannelBlockKey& key, uint seed = 0)
	{
		//全局 qHash 须显式限定：本命名空间内一旦声明自定义重载，无限定调用
		//只查到 PostCurve::qHash（基础类型无 ADL 关联命名空间）
		uint h = ::qHash(key.subcaseId, seed);
		h = ::qHash(key.schema, h);
		return ::qHash(key.pointId, h);
	}

	class POSTCURVE_EXPORT mCurveDataSourceStore : public QObject
	{
		Q_OBJECT
	public:
		static mCurveDataSourceStore* getInstance();

		mCurveDataSourceStore(const mCurveDataSourceStore&) = delete;
		mCurveDataSourceStore& operator=(const mCurveDataSourceStore&) = delete;

		//--- 数据源文件类型（R22）---
		//数据源类型：Pch = .pch（PchReader 解析）；AsciiColumn = .csv/.txt 通用
		//列数据（mCurveAsciiReader 解析为具名数值列）。登记/装配按此分发
		enum class DataSourceKind
		{
			Pch,
			AsciiColumn
		};

		//按扩展名判定文件类型：.csv/.txt（不分大小写）-> AsciiColumn，
		//其余（含 .pch 与一切未知扩展名）-> Pch（与既有 pch 唯一入口兼容）
		static DataSourceKind fileKind(const QString& filePath);

		//登记键规范化：QFileInfo::canonicalFilePath（文件不存在时退回
		//absoluteFilePath）。addFile/contains/file/index 一律先过本函数再查键，
		//避免同一文件因路径拼写不同（相对/大小写/短路径）登记两份，以及
		//曲线 sourceFile 按旧拼写回查时对不上键
		static QString normalizePath(const QString& filePath);

		//解析并登记一个数据集（按 fileKind 分发：pch 经 PchReader；csv/txt 经
		//mCurveAsciiReader，见 R22-T1）。键为 normalizePath 规范化路径；已存在
		//时幂等成功（返回 true，不重复解析）；解析异常或无有效数据返回 false，
		//原因经 err（可为空）带出。
		//同步阻塞语义保留（会话恢复/批量模板应用等无人值守路径专用），内部
		//调用点零改动；与后台异步导入重叠时在主线程等闸门（罕见，见
		//addFileAsync 注释）
		bool addFile(const QString& filePath, QString* err = nullptr);

		//异步导入受理三态（addFileAsync 返回）：
		//  AlreadyRegistered  已登记（_files 命中）——仓库不发任何导入信号，
		//                     调用方走原"已存在"内联成功链路（现状幂等语义）
		//  Accepted           已受理入队/开始解析——调用方挂 sig_importFinished
		//                     续接（成败以信号为准，本返回值仅表示受理成功）
		//  DuplicateInFlight  同文件已在解析中/排队中——幂等 no-op 不重复入队
		//                     （D2），后续成败仍经 sig_importFinished 通知
		enum class AddFileAsyncResult
		{
			AlreadyRegistered,
			Accepted,
			DuplicateInFlight
		};

		//异步受理一个文件导入（交互入口专用）：解析在后台串行执行（FIFO 队列
		//+闸门，同一时刻至多一个文件在解析；pch/ascii 按 fileKind 分发到各自
		//解析器），完成后主线程提交（写 _files 或 _asciiFiles + buildIndex）
		//并发 sig_fileAdded。已登记文件幂等返回 AlreadyRegistered 且不发任何
		//导入信号；解析成败（含"文件无有效数据块/数据行"等与同步路径一致的
		//err 文案）经 sig_importFinished 通知
		AddFileAsyncResult addFileAsync(const QString& filePath);

		//导入活动规模：解析中(1) + 排队中数量（进度对话框"第 i/N 个"与
		//协调器队列生命周期判定用）
		int importQueueDepth() const;

		bool contains(const QString& filePath) const;   //pch 与 ascii 数据集一并覆盖
		QStringList filePaths() const;                                 //登记顺序
		const PchReader::PchFile* file(const QString& filePath) const; //未登记/非 pch 返回 nullptr
		const ListWgtStruct* index(const QString& filePath) const;    //未登记返回 nullptr

		//--- csv/txt 通用列数据（R22，下游 UI 依赖的对外签名）---
		//ascii 数据集固定语义常量（Types/Requests 列唯一项与装配五元组常量）：
		//"结果类型"显示文本 = "Unknown"（实际结果类型语义，非横坐标——
		//isChannelSchema("Unknown")==true、isVectorSchema("Unknown")==false）；
		//"对象"显示文本 = "Block 1"；工况/对象内部标识恒 1（UI 不显示工况级）
		static QString asciiSchemaDisplayText();
		static int asciiSubcaseId();
		static int asciiPointId();
		static QString asciiObjectDisplayText();
		//分量列名（显示序，即 Components 列表内容；同名列不去重、按首个匹配
		//下标取列）。未登记 / 非 ascii 文件返回空
		QStringList asciiColumnNames(const QString& filePath) const;

		//整体清空已登记数据集（会话恢复前的重置用；曲线数据本体随文档快照
		//内嵌恢复，此处清的是可重解析的溯源缓存）
		void clear();

		//--- 分量名规则（共享） ---
		//结果类型是否节点矢量（仅矢量结果有分量展开，随导入列表联动语义）
		static bool isVectorSchema(const QString& schema);
		//结果类型是否横坐标类型（"Time[Sec]"/"Frequency[Hz]"，buildIndex 生成，
		//随文件数据形态；"定义曲线"isAbscissaSchema/参数页 isAbscissaType 同义）
		static bool isAbscissaSchema(const QString& schema);
		//侧来源是否通道形态（实际结果类型，取通道分量值）：非空且非横坐标类型，
		//与 isAbscissaSchema 互补（空=未记录，随锚通道走）。"Y 侧换源 X 保持
		//自身形态"的 xChannelMode 判定统一走这里
		static bool isChannelSchema(const QString& schema);
		//工况显示文本（"从文件创建曲线"/"定义曲线"工况列同源，内容一致）：
		//"Subcase {id} (label)"，label 取该工况数据块中首个非空 label 值文本
		static QString subcaseDisplayText(const PchReader::PchFile& file, int subcaseId);
		//上一函数的实例级路径版（回显统一入口）：按 fileKind 分发——ascii 文件
		//返回空串（UI 隐藏工况级；标签模板 {y.MXSubcase} 解析为空，符合"占位符
		//来源缺失替换为空"既有规则）；pch 文件同静态版语义；未登记返回空串
		QString subcaseDisplayText(const QString& filePath, int subcaseId) const;
		//数值格式 -> 分量名列表（频响 12 个 / 瞬态 6 个；未识别格式返回空）。
		//isTime 为块级瞬态标志（ListWgtStruct::ycomponent 叶子随格式下发，数据形态
		//判定），调用方勿再按文件级 xresulttype 反推
		static QStringList expandedComponents(const QString& numericFormat, bool isTime);
		//分量名 -> 数据下标：0-5 = primary（MAG/REAL/瞬态值），6-11 = secondary
		//（PHA/IMAG）；未识别返回 -1
		static int componentIndex(const QString& component);
		//复数配对分量（同方向令牌互换）：MAG|X-Trans ↔ PHA|X-Trans、
		//REAL|X-Trans ↔ IMAG|X-Trans；非复数分量返回空串。
		//复数曲线成对换源（"更改曲线"任一侧）取对侧分量用
		static QString siblingComplexComponent(const QString& component);

		//--- 通道数据装配（"从文件创建曲线"/"定义曲线"/"更改曲线"共用，规则唯一实现处） ---
		//装配结果：X/Y 数据序列 + 防线告警标志（本类纯计算不依赖控制器，
		//如何上报由调用方决定）
		struct ChannelDataResult
		{
			QVector<double> xData;
			QVector<double> yData;
			bool skippedNoSecondary = false;   //有块因缺虚部/相位数据被跳过（防线触发）
		};

		//按通道五元组（id 形态）从已解析数据装配 X/Y 序列：
		//  - blocks 按 subcaseId/schema/pointId 匹配（同键多块依次拼接）；
		//  - 防线：PHA/IMAG 分量（componentIndex>=6）遇到无 secondary 的瞬态
		//    实数块整块跳过（不填 0 产出全零假曲线），置 skippedNoSecondary；
		//  - 其余按 componentIndex 取 primary（0-5）/secondary（6-11）；
		//  - X = record.abscissa（频率/时间）。
		//分量未识别 / 无匹配块 / 无有效数据点返回 false（err 带原因）。
		//file 入参即 file() 返回的解析数据；本函数纯计算，不触碰仓库状态
		static bool assembleChannelData(const PchReader::PchFile& file,
			int subcaseId, const QString& schema, int pointId, const QString& component,
			ChannelDataResult* out, QString* err = nullptr);

		//上一函数的实例级索引化重载（optimize-channel-map-perf）：按登记键直取
		//_blockIndexes 中匹配块（不再全块线性扫），装配体与输出与静态版逐位一致
		//（索引内块序 = buildIndex 遍历 file.blocks 序）。
		//R22 起 ascii 文件（fileKind==AsciiColumn）走独立分支：component 按
		//列名首个匹配取列下标，yData=列值、xData=行号 1..N（默认横坐标；UI 侧
		//X 通道另经本函数以列名装配）、skippedNoSecondary 恒 false；未登记/
		//列名不存在按既有 err 风格返回 false。未登记路径返回 false
		bool assembleChannelData(const QString& filePath,
			int subcaseId, const QString& schema, int pointId, const QString& component,
			ChannelDataResult* out, QString* err = nullptr);

		//通道存在性探针（试装配的布尔替身，optimize-channel-map-perf）：与静态
		//assembleChannelData 的成败判定严格互充——componentIndex 有效 ∧ ∃匹配块
		//（records 非空 ∧ (idx<6 ∨ 块内存在 secondary)）。不复制任何数据。
		//err 语义与装配一致（未识别分量/不在仓库/无有效数据点）。
		//R22 起 ascii 文件走独立分支：列名在已登记 ascii 数据集中即存在
		bool hasChannelData(const QString& filePath,
			int subcaseId, const QString& schema, int pointId, const QString& component,
			QString* err = nullptr) const;

		//按方向装配通道“幅值”序列（动刚度/VTF/NTF 实测曲线用）：随首匹配块的
		//数值格式分流——MAGNITUDE-PHASE 取 MAG 分量；REAL-IMAGINARY 取 REAL/IMAG
		//两路合成 sqrt(re^2+im^2)（动刚度 1/实部 会在实部过零点爆尖峰，必须用
		//幅值）；REAL-ONLY（瞬态/标量声压等）取 REAL 分量原值。
		//direction 为方向令牌（"X-Trans"/"FX" 等，即分量令牌 '|' 后半）。
		//无匹配块/无有效数据点返回 false（err 带原因）
		static bool assembleChannelMagnitude(const PchReader::PchFile& file,
			int subcaseId, const QString& schema, int pointId, const QString& direction,
			ChannelDataResult* out, QString* err = nullptr);

	signals:
		//新数据集登记完成（解析+索引就绪），订阅方据此重建数据集界面
		void sig_fileAdded(const QString& filePath);

		//—— 异步导入生命周期（仅 addFileAsync 受理的路径发；filePath=规范化键，
		//与 sig_fileAdded 同键）——
		//解析开始（受理后派发的同周期内，主线程）；totalBytes=文件字节，
		//不可得时为 0（不定进度）
		void sig_importStarted(const QString& filePath, qint64 totalBytes);
		//解析进度（解析器内 50ms 节流 + 区间边界强制上报，经队列信号回主线程；
		//bytesDone 单调不减，bytesTotal 与 sig_importStarted 一致）
		void sig_importProgress(const QString& filePath, qint64 bytesDone, qint64 bytesTotal);
		//解析完成（主线程提交后；ok=false 时 err 与同步 addFile 的 err 文案同源）。
		//注意：会话恢复触发 clear() 作废导入（D3）时结果被丢弃，本信号不发出，
		//代之以 sig_importAborted
		void sig_importFinished(const QString& filePath, bool ok, const QString& err);
		//导入作废（clear() 时有在跑/排队导入）：进度对话框与状态栏导入消息
		//立即收尾用（02 §3.3"clear() 作废→对话框立即关闭"）；入口不订阅本信号
		void sig_importAborted();

	signals:
		//内部信号：闸门放行（每次 worker 完成回调收尾时发出，无论提交或作废；
		//供同步 addFile 的嵌套等待退出——本类成员内连接，不外泄）
		void sig_parseIdle();

	private:
		explicit mCurveDataSourceStore(QObject* parent = nullptr);
		~mCurveDataSourceStore() override;

		//后台纯解析 worker（仅 store.cpp 可见；只持有入参与出参，不触 store
		//状态与任何 GUI 对象；moveToThread 到导入线程）
		class ImportWorker;

		//导入作业（FIFO 队列元素；仅主线程读写）
		struct ImportJob
		{
			QString key;          //normalizePath 规范化键
			QString displayPath;  //原始路径（进度对话框文件名显示用）
			DataSourceKind kind = DataSourceKind::Pch;   //解析分发（worker 槽二选一）
		};

		//按规范化键派发下一个排队作业（闸门空闲时；无作业/在跑则 no-op）
		void startNextJobIfIdle();

		//worker 完成（队列信号回主线程）：提交或作废，随后放行闸门。
		//H1：结果载体为 shared_ptr（排队连接零深拷贝），槽内 move 进 _files。
		//ascii 版（R22）同构，结果 move 进 _asciiFiles
		void onParseFinished(const QString& key, bool ok, const QString& err,
			std::shared_ptr<PchReader::PchFile> file);
		void onAsciiParseFinished(const QString& key, bool ok, const QString& err,
			std::shared_ptr<AsciiColumnFile> file);

		//worker 进度（队列信号回主线程）：作废/非在跑作业丢弃 + 单调钳制后
		//以 sig_importProgress 对外转发
		void onParseProgress(const QString& key, qint64 bytesDone, qint64 bytesTotal);

		//按 blocks 建层级索引（原 mOpenCurveDlg::setListWgtStruct 逻辑原样迁移）
		void buildIndex(const QString& filePath);

		static mCurveDataSourceStore* _instance;   //单例

		QHash<QString, PchReader::PchFile> _files;   //规范化路径 -> 原始解析数据
		QHash<QString, AsciiColumnFile> _asciiFiles; //规范化路径 -> ascii 列数据（与 PchFile 分存，R22）
		QHash<QString, ListWgtStruct> _indexes;      //规范化路径 -> 层级索引
		//块索引（optimize-channel-map-perf）：规范化路径 -> (工况,结果类型,对象) ->
		//块下标（指向 _files 该文件 blocks 的 vector 位置；解析后不再修改，索引
		//稳定；块序 = buildIndex 遍历序，与线性扫命中序一致）
		QHash<QString, QHash<ChannelBlockKey, QVector<int>>> _blockIndexes;
		//路径规范化缓存（optimize-channel-map-perf）：raw -> canonical（clear 一并清）
		QHash<QString, QString> _canonCache;
		QStringList _order;                          //登记顺序（QHash 键序未指定，另存列表保证 file 下拉稳定）

		//—— 异步导入调度（以下成员仅主线程读写；worker 只拿到 key/路径/选项与出参）——
		ImportWorker* _worker = nullptr;   //后台解析对象（moveToThread(_thread)）
		QThread* _thread = nullptr;        //导入线程（store 单例生命周期=进程，随 aboutToQuit 收尾）
		QVector<ImportJob> _pending;       //FIFO 队列
		bool _running = false;             //闸门：同一时刻至多一个文件在解析
		QString _runningKey;               //当前在跑文件的键（幂等/进度钳制用）
		quint64 _sessionGeneration = 0;    //会话代次：clear() 时 +1；提交前校验不匹配即丢弃（D3）
		quint64 _runningGeneration = 0;    //在跑作业启动时的会话代次快照
		qint64 _lastProgressDone = -1;     //在跑文件已上报的最大进度字节（多线程观测乱序钳制）
		std::atomic<bool> _cancelCurrent { false };   //退出快速中止标志（R3，注入 reader cancelFlag）
	};
}
