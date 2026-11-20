#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 定义曲线对话框（功能弹窗，单例）
//
// 入口：Ribbon"曲线"页 -> "曲线"组 -> "定义曲线"按钮（checkable 模式开关，
// 当前激活视口为曲线视窗时可用；点击 -> mCurveViewportFactory::setDefineCurveMode，
// 显隐/目标重绑跟随 sig_defineCurveStarted/sig_defineCurveStopped）。与编辑
// 曲线/基准线/注释互斥——互斥在视口工厂收口，开启任一功能自动退出其余。
// 开启时以当前激活视口承载的图表为目标；模式存活期间目标不随视口切换
// 漂移，目标图表（或其祖先页）被删除时自动关闭（mApplyStyleDlg 先例）。
//
// 形态：
//   左栏"曲线列表"：曲线名称复选框 + 名称编辑框 + 待新增曲线列表 +
//     剪切/复制/粘贴/新增按钮——视口有曲线时打开即把图表内全部曲线载入为
//     条目（树序，打开前选中曲线为初始选中，见 onDefineStarted），否则为
//     初始态（列表为空，仅"新增/关闭"可用，其余全部置灰；未应用的定义
//     打开时直接丢弃），点"新增"直接追加"曲线 N"空条目（不打扰确认），
//     右栏向量/来源定义区随选中条目联动；
//   右栏"曲线定义"：
//     向量 X/Y 单选 + 回显编辑框（X=横坐标/引用，Y=分量/表达式）；
//     来源 文件/数学运算/数值 单选切换三张定义页——常规 XY 与复数图目标来源
//     按向量侧独立（每侧各自来源；文件路径下拉也按侧独立，每侧各自
//     filePath）。R21 交互模型：页面只随"来源"单选切换；点向量=选中该侧
//     并套用当前页来源——同源仅切焦点（文件页四级列编辑目标/数学页插入与
//     拾取目标/数值页激活列），异源按当前页来源转换该侧（与点"来源"单选
//     同链路，脏确认取消则两单选组一并回退），永不跳页——非柱状下"激活侧
//     来源==来源单选"恒成立；柱状图目标维持整条来源语义（共享数值表不变）：
//       文件     "选择数据文件"行（全路径下拉 + 文件夹按钮，共享数据集
//                mCurveDataSourceStore）+ 四级联动列（工况/结果类型/对象/
//                分量，列样式照"从文件创建曲线"：灰底标题条（标题+1/N计数）
//                + 边框列表容器，工况文本与"从文件创建曲线"同源一致）。
//                向量 X/Y 单选切换四级列的编辑目标，两轴相互独立（工况等
//                各级选择互不相通）：Y 侧主通道必选，X 侧可选——未选时
//                X=数据块横坐标，选中通道时取该通道分量值作 X（点数与 Y
//                一致才配对）；X 侧结果类型照"从文件创建曲线"X 轴数据源：
//                横坐标类型置首，并加入 Y 侧工况的结果类型（去重）。X/Y 回
//                显=文件:对象/分量（X 未选时为
//                文件:对象/结果类型）；数据装配与"从文件创建曲线"一致；
//       数学运算  表达式构建器（图3）：函数/曲线选项卡（函数库搜索 +
//                名称/值参数表 / 文档 页->视口->曲线->x/y 引用树）、
//                直接插入/用于表达式、表达式编辑框、计算求解复选框、
//                值预览表；曲线向量引用形如 p1w1c1.y（按文档结构解析，
//                求值经 mCurveExpressionEvaluator：自研引擎 + 按x插值）。
//                X 与 Y 交互对称（来源切数学运算后均可）：拾取默认激活，到视
//                窗 Shift+左键点曲线即回填引用到激活向量侧（X=.x 整体替换，
//                Y/yp/ym=.y 光标处插入；点击输入框可切换目标侧），也可经下方
//                表达式构建器插入——"曲线"页选中曲线"直接插入/用于表达式"
//                按当前向量侧补 .x/.y（函数参数格 fx 弹曲线选择器双击插入）；
//                函数页仅可插入 Y 侧表达式（X 为单引用槽，函数调用串作 X 无效）；
//                复数图目标两按钮分流：直接插入按激活向量侧直落 x/yp/ym 框
//                （yp/ym 各为独立表达式槽，互不镜像），用于表达式只落
//                "表达式"框（预览跟随表达式框）；
//       数值     表格逐行输入（截图）：常规 XY 为 Pt|X|Y、复数图为 Pt|X|Yp|Ym
//                （Pt/Index 列只读行号）——仅"数值来源侧"的列可编辑，其余列
//                只读回显该侧当前来源装配的实际数据（'f' 6，行位对齐、超出
//                留空）；剪切/复制/粘贴只作用于可编辑列；
//                柱状图目标为共享数值表（设计图）：左栏列表首行挂"Categories"
//                条目，表格为 Index + Categories + 每条目一列（列头=条目显示
//                名，随名称联动）——Categories 列全条目共享，点"新增"直接在
//                表尾右扩新曲线列（不弹"不保留当前数据"确认，已有行数据保
//                留）；行=数据点（+/-增删行全列联动）；
//   底部：关闭 / 应用——应用把列表条目整批写入目标图表（beginMacro 一次
//     Ctrl+Z 整批撤销），成功后条目留存不清空（新建条目绑定生成曲线，再次
//     应用原位更新不重复新建），可一直修改。
//
// 形态：照 mApplyStyleDlg/mPropertyEditorPopup 的无边框白卡片（M徽标标题栏+✕，
// 非模态、可拖动缩放），DlgGeom::bind 记忆大小/位置；父窗口沿 parentWidget 链
// 走到顶层主窗口（QMdiSubWindow 亦带 Window 标志，直接作父会被 MDI 收编）。
// 曲线写入链路与"使用当前绘图"并入一致：addCurve + setSeriesData +
// setSeriesSource + 轮换配色（从目标图表既有曲线数续起）。
// =============================================================================

#include "PostCurveGlobal.h"
#include "mCurveExpressionEvaluator.h"

//QT
#include <QWidget>
#include <QList>
#include <QPair>
#include <QPoint>
#include <QString>
#include <QVector>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QMouseEvent;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;
class QResizeEvent;
class QSizeGrip;
class QStackedWidget;
class QTabWidget;
class QTableWidget;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class QWidget;

namespace PostCurve
{
	//定义中的曲线（对话框会话内的一条待新增条目；应用前只存在于本对话框）
	struct DefineCurveEntry
	{
		//文件来源的单侧通道选择（X/Y 轴相互独立，工况等各级选择互不相通）
		struct FileChannel
		{
			int subcaseId = 0;          //0=未选
			QString schema;             //结果类型（X 侧可为横坐标类型）
			int pointId = 0;            //对象 id，0=未选
			QString objectText;         //对象显示名（"Point id n"）
			QString component;          //分量名

			bool complete() const
			{
				return subcaseId > 0 && !schema.isEmpty() && pointId > 0 && !component.isEmpty();
			}
		};

		//左栏
		bool useName = true;        //"曲线名称"复选框：勾选=用名称编辑框文本命名
		QString name;               //曲线名称（新增时默认"曲线 N"）

		//向量
		int activeVector = 1;       //当前编辑向量：0 X 1 Y/yp 2 ym（复数图目标三槽，
		                            //默认 Y=主通道；文件来源兼作四级列编辑目标侧，
		                            //数学来源兼作插入目标）
		QString xText;              //X 回显：文件=文件:对象/分量或结果类型；数学=曲线向量引用
		QString yText;              //Y 回显：文件=文件:对象/分量；数学=表达式
		QString ypText;             //复数图相位侧：文件=文件:对象/PHA|方向 回显；数学=相位表达式
		QString ymText;             //复数图幅值侧：文件=文件:对象/MAG|方向 回显；数学=幅值表达式

		//来源：0 文件 1 数学运算 2 数值。柱状图目标保留整条 source 语义不拆
		//（共享数值表/四级列/表达式等行为零变化）；常规 XY 与复数图目标来源
		//按向量侧独立（xSource/ySource/ypSource/ymSource，"来源"单选只写激活
		//侧，点向量=选中该侧并套用当前页来源——激活侧来源==来源单选恒成立，
		//R21）——source 此时
		//兼任"最近一次显式选择的镜像"（sideSource 的有效来源回退值），非柱状
		//路径不直读它判定
		int source = 0;
		//按侧来源哨兵（R20-B01）：-1=未显式设置（出厂"文件"不是用户对该侧的
		//选择，不得按它把用户带离当前来源页）——有效来源经 sideSource 取：
		//未设置侧跟随整条 source 镜像，首次显式选择即全侧落定（onSourceChanged），
		//此后各侧只随各自的显式切换分叉；编辑模式载入写入的都是显式值(≥0)；
		//粘贴/切型播撒不注入哨兵，显式值得以保留
		int xSource = -1;           //X 侧来源（-1=未显式设置）
		int ySource = -1;           //Y 侧来源（复数图目标=yp 相位侧；-1=未显式设置）
		int ypSource = -1;          //复数图相位侧来源（-1=未显式设置）
		int ymSource = -1;          //复数图幅值侧来源（-1=未显式设置）

		//文件来源（通道按侧独立现状保持，四级列随向量单选切换编辑目标；
		//复数图目标三槽 x/yp/ym 各自保存，yp/ym 分别落在相位/幅值曲线）。
		//文件路径下拉同样按向量侧独立：每侧各自 filePath，切向量时下拉回显
		//该侧文件（两侧可选不同文件；柱状图目标仍用整条 filePath）
		QString filePath;
		QString xFilePath;          //X 侧数据文件
		QString yFilePath;          //Y 侧数据文件（复数图目标=yp 相位侧）
		QString ypFilePath;         //复数图相位侧数据文件
		QString ymFilePath;         //复数图幅值侧数据文件
		FileChannel xChannel;       //X 侧通道（未选时 X=数据块横坐标）
		FileChannel yChannel;       //Y 侧主通道（必选）
		FileChannel ypChannel;      //复数图相位侧通道（复数图目标时使用）
		FileChannel ymChannel;      //复数图幅值侧通道（复数图目标时使用）

		//数学运算来源
		QString expression;         //Y 表达式（XY 目标 Y 侧数学内容，常驻免暂存；
		                            //复数图目标为"表达式"框独立暂存区）
		bool solve = true;          //计算求解：勾选=按X采样求值；不勾=表达式须为单引用直接取值

		//数学内容暂存（按向量侧独立，沿用 hasMathStash 思路按侧置位）：离开
		//该侧数学来源时暂存回显，切回恢复。XY 的 Y 框与表达式框同源免暂存
		//（expression 常驻），仅 x/yp/ym 侧需要
		QString mathXRef;           //数学 X 曲线引用（xText 的数学形态）
		QString mathYpText;         //复数图相位表达式暂存
		QString mathYmText;         //复数图幅值表达式暂存
		bool hasXMathStash = false;   //X 侧离开数学运算时置位（切回有暂存可恢复）
		bool hasYpMathStash = false;  //复数图相位侧暂存标志
		bool hasYmMathStash = false;  //复数图幅值侧暂存标志

		//数值来源
		QString categoriesText;     //逗号/空白分隔刻度名（横坐标类别名，与数值按行对应）
		QString valuesText;         //逗号/空白分隔数值，X 按索引 0..N-1
		//数值来源（复数图目标）：表格 X/Yp/Ym 三列按行对应的分隔数值串
		//（Pt=行号自动；Ym 落上面板幅值，Yp 落下面板相位）
		QString pairXText;          //X 列
		QString pairYpText;         //Yp 列（相位侧）
		QString pairYmText;         //Ym 列（幅值侧）
		//数值内容脏标记按向量侧持有（该侧改格/增删行置位）——离开"数值"的
		//确认框按侧判定；柱状图目标（整条数值）映射到 yValueDirty
		bool xValueDirty = false;   //X 侧数值内容被用户改过（自动装配不算改动）
		bool yValueDirty = false;   //Y（复数图 yp）侧数值内容被用户改过
		bool ypValueDirty = false;  //复数图相位侧数值内容被用户改过
		bool ymValueDirty = false;  //复数图幅值侧数值内容被用户改过

		//复数图目标（数学来源）成对命名风格：0 P/M（-MAG|Y/-PHA|Y）1 R/I（-REAL|Y/-IMAG|Y）；
		//文件来源由所选通道分量自然命名，不受此影响
		int pairStyle = 0;

		//编辑模式（设计图：视口有曲线点"定义曲线"载入当前曲线）：>=0 = 绑定的
		//既有复数曲线节点 id，应用时原位更新该曲线（不新建）；-1 = 新建条目
		//（粘贴条目自动解除绑定）
		int editCurveId = -1;

		bool fileComplete() const { return !filePath.isEmpty() && yChannel.complete(); }
	};

	class POSTCURVE_EXPORT mDefineCurveDlg : public QWidget
	{
		Q_OBJECT
	public:
		//单例访问（Ribbon 点击时先就位实例——ctor 已连工厂模式信号，显隐随
		//模式；parent 仅首次创建使用，空则取当前活动顶层窗口）
		static mDefineCurveDlg* getInstance(QWidget* parent = nullptr);

		~mDefineCurveDlg() override;

		mDefineCurveDlg(const mDefineCurveDlg&) = delete;
		mDefineCurveDlg& operator=(const mDefineCurveDlg&) = delete;

	private:
		explicit mDefineCurveDlg(QWidget* parent);

		//--- UI 构建 ---
		void buildUi();
		QWidget* buildListPanel();      //左栏：曲线列表
		QWidget* buildRightPanel();     //右栏：向量/来源/定义页
		QWidget* buildFilePage();       //来源页：文件（数据集四级联动）
		QWidget* buildMathPage();       //来源页：数学运算（表达式构建器）
		QWidget* buildValuePage();      //来源页：数值序列

		//--- 条目与状态 ---
		QString nextCurveName() const;                          //新增默认名"曲线 N"（N 会话内自增）
		int currentEntryIndex() const;                          //当前选中条目下标（无选中 -1）
		QString entryAutoName(const DefineCurveEntry& e) const; //未勾"曲线名称"时的自动命名
		bool entryComplete(const DefineCurveEntry& e) const;    //条目定义是否完整（应用可用性）
		//--- 按向量侧的状态访问（vector: 0=x 1=Y/yp 2=ym；柱状图目标退化为
		//--- 整条语义——sideSource/sideFilePath 映射 e.source/e.filePath，其余
		//--- 映射 Y 侧字段，保证柱状路径行为与拆分前逐字一致） ---
		int sideSource(const DefineCurveEntry& e, int vector) const;
		void setSideSource(DefineCurveEntry& e, int vector, int source) const;
		QString sideFilePath(const DefineCurveEntry& e, int vector) const;
		void setSideFilePath(DefineCurveEntry& e, int vector, const QString& path) const;
		DefineCurveEntry::FileChannel& sideChannel(DefineCurveEntry& e, int vector) const;
		const DefineCurveEntry::FileChannel& sideChannel(const DefineCurveEntry& e, int vector) const;
		QString& sideValueText(DefineCurveEntry& e, int vector) const;         //该侧数值串
		const QString& sideValueText(const DefineCurveEntry& e, int vector) const;
		bool sideValueDirty(const DefineCurveEntry& e, int vector) const;
		void setSideValueDirty(DefineCurveEntry& e, int vector, bool dirty) const;
		QString sideMathText(const DefineCurveEntry& e, int vector) const;     //该侧数学内容
		bool sideFileComplete(const DefineCurveEntry& e, int vector) const;    //该侧文件通道完整
		bool sideHasContent(const DefineCurveEntry& e, int vector) const;      //该侧任一来源有内容
		void refreshActionStates();     //按钮/面板整体启停
		void loadEntryToPanel();        //条目 -> 右栏/名称区回填
		void syncPanelToEntry();        //面板控件值 -> 当前条目（切换/应用前兜底收集）
		void clearPanelState();         //无选中条目时清空右栏
		void resetDialog();             //复位到初始态（模式开启/重新打开时，应用后不清空）
		void setPickingVector(int vector);  //曲线拾取目标侧切换（数学来源默认激活；-1 退出）

		//--- 来源切换共用链路（非柱状，R21 交互模型）---
		//"离开数值"脏确认框（dirty=false 直接通过；返回 false=用户取消）
		bool leaveValueConfirmed(bool dirty);
		//把 side 侧来源切换为 id：脏确认→离开数学暂存→B02 倒表→B03 x 快照→
		//落值+镜像→落定（B01）→激活侧=side→切页/回显。"来源"单选点击（side=
		//当前激活侧）与向量单选按当前页来源转换该侧（side=新选中侧，页面只随
		//来源单选不跳页）共用；返回 false=用户取消（已回退来源+向量两个单选
		//组，状态零变化）
		bool applySourceChange(int side, int id);

		//--- 左栏 ---
		void addEntry();                //"新增"
		void removeEntry(int row);      //"剪切"删除条目
		void updateListItemText(int row);   //名称/来源变化同步列表行文字

		//--- 文件来源页（四级联动） ---
		void fillFileCombo();           //数据文件下拉（共享数据源仓库）
		void fillSubcases();            //工况列表 <- 当前数据文件
		void fillSchemas();             //结果类型列表 <- 当前工况（仅矢量结果）
		void fillObjects();             //对象列表 <- 当前结果类型
		void fillComponents();          //分量列表 <- 当前对象（分量展开规则同"从文件创建曲线"）
		void refreshFileVectorTexts();  //按条目文件通道选择回填 X/Y 向量文本
		void refreshFileColumns();  //四级列按当前向量侧的通道选择回填
		DefineCurveEntry::FileChannel& activeFileChannel(DefineCurveEntry& e);        //向量单选侧的通道
		const DefineCurveEntry::FileChannel& activeFileChannel(const DefineCurveEntry& e) const;
		int xCascadeSubcase(const QString& schema) const;     //X 侧对象/分量级联工况（含回退）
		void refreshXColumnStates();  //X 侧选横坐标类型时对象/分量列置灰
		//ascii 列数据集（csv/txt，R22）通道默认补全：[Unknown]/[Block 1] 恒选、
		//X 侧分量默认第 1 列（替代 pch 的"数据块横坐标"默认）；仅补空缺值。
		//vector=当前四级列编辑目标侧（0=X），pch 路径不动作
		void completeAsciiChannel(DefineCurveEntry::FileChannel& ch, int vector);
		void refreshSubcaseColumnVisible();  //工况列容器随当前侧文件类型显隐（ascii 隐藏）

		//--- 数学运算页 ---
		void rebuildObjectTree();       //曲线引用树（文档 页->视口->曲线->x/y，p/w/c 编号）
		void rebuildFuncTree();         //函数树（函数表按分类分组）
		void applyFuncTreeFilter();     //搜索/分类过滤函数树
		void fillFuncParams(const QString& funcName);  //名称/值参数表（分组+参数 fx 选择器）
		QString composeFuncCall() const;  //按参数值现合成函数调用串（未选函数返回空）
		void refreshInsertButtons();    //直接插入/用于表达式 启停
		QString currentInsertText() const;  //当前可插入文本（函数合成串或曲线引用）
		void insertIntoExpression(const QString& text);  //插入到表达式光标处并同步
		void refreshPreview();          //值预览（当前条目来源数据前几个点）
		void refreshVectorEditability();  //向量框按来源与激活侧启停
		void refreshValueVectorTexts(); //数值来源 X/Y 回显文本

		//--- fx 曲线选择器（名称/值参数格） ---
		void openRefPicker(QLineEdit* cellEdit);
		void applyPickedRef(QLineEdit* cellEdit, const QString& ref);

		//--- 数值来源 ---
		static QList<double> parseValues(const QString& text);
		bool targetIsBarChart() const;           //目标图表为柱状图（向量区按此切换形态）
		bool targetIsComplexChart() const;       //目标图表为复数图（向量三槽 x/yp/ym）
		bool categoriesRowActive() const;        //柱状图左栏"Categories"行选中态
		int entryToListRow(int entryIndex) const;   //条目下标 -> 列表行（柱状图首行=Categories）
		int listRowToEntryIndex(int listRow) const; //列表行 -> 条目下标（Categories 行=-1）
		void syncBarListShape();                 //左栏 Categories 行随图表类型增删
		QString entryDisplayName(int entryIndex) const;  //条目显示名（列表/数值列表头同源）
		void readValueTableIntoEntry(DefineCurveEntry& e) const;  //常规表可编辑列（数值侧）按行读回条目
		void fillValueTableSides(const DefineCurveEntry& e);     //非柱状数值表按各侧来源展开（数值侧可编辑/他侧只读回显）
		void computeSideEchoTokens(const DefineCurveEntry& e);   //非数值侧列回显 token 现算缓存（'f' 6）
		void applySideCellStates(int row);                       //行内格可编辑性/只读回显收口（行插/回填后调）
		bool isEditableValueColumn(int col) const;               //数据列是否可编辑（数值侧列；柱状=数值列全可编辑）
		QList<int> valueClipColumns() const;                     //剪切/复制/粘贴作用的列（只含可编辑列；柱状=Categories+数值列现状）
		int firstEditableValueColumn() const;                    //首个可编辑数据列（新行落焦点用；无可编辑列返回 1）
		void insertXYValueRow(int row, const QString& x, const QString& y);   //XY 数值表插行（Pt 只读行号 + X/Y 数值格）
		void insertValueRow(int row, const QString& name, const QString& value); //柱状共享表插行（Categories 列 + 各条目数值列）
		void insertComplexValueRow(int row, const QString& x, const QString& yp,
			const QString& ym);   //复数图数值表插行（Pt 只读行号 + X/Yp/Ym 数值格）
		void renumberValueRows();                 //Index 列按 0..N-1 重排（插/删/贴后调用）
		QList<int> selectedValueRows() const;     //选中行号升序（整行选择）
		void refreshValueToolBarStates();         //行操作工具条启停（删除钮随选中行/清空钮随表行数）
		void commitValueTableToEntry();           //表格读回条目（柱状图=共享列全量落库）+ 向量回显 + 按钮态
		void markValueCellValidity(int row, int col); //非法数值格红字提示（对齐预览错误配色）
		void refreshValueColumnHeader();          //值列表头=条目显示名（随曲线名称联动）
		void markValueRowOpDirty(DefineCurveEntry& e);   //行级操作（增/删/清空/插行）置脏：全部数值侧（柱状=y 侧）
		//柱状图共享数值表（设计图）：Index + Categories + 每条目一列（列头=
		//条目显示名）；Categories 列全条目共享，点"新增"即在表尾右扩一列，
		//已有行数据保留
		void syncValueTableShape();               //列数/表头/列宽随模式与条目集
		void rebuildValueTableView();             //数值表按当前模式整表重建（类型切换/复位）
		void fillBarColumnFromEntry(int entryIndex); //共享表：单条目数值列回填（行不足自动扩行）

		//--- 引用解析（文档结构 p页w视口c曲线.[x|y]） ---
		bool resolveRef(const QString& ref, int* curveId, bool* isX, QString* normalized) const;
		bool buildRefData(const QString& ref, CurveRefData* out, QString& errMsg) const;

		//--- 应用 ---
		bool applyEntries();
		void addEntryWithStyle(int pairStyle);   //新增条目（pairStyle=复数对命名风格）
		//编辑模式：既有曲线载入为单条目（复数=三壳曲线 / XY=普通曲线 /
		//柱状=柱状曲线；来源/向量随模型溯源字段还原；数值来源倒出当前数据
		//进表），应用时原位更新该曲线节点
		void addEntryFromCurve(int curveId);
		void dumpCurveValuesToEntry(int curveId, DefineCurveEntry& e) const;
		//复数对防呆：幅值槽应持 MAG/REAL 令牌、相位槽应持 PHA/IMAG 令牌；
		//文件来源两侧选反时自动对调 ym/yp 通道（含回显文本），防两侧语义颠倒
		void normalizeComplexPairChannels(DefineCurveEntry& e) const;
		bool buildFileData(const DefineCurveEntry& e, QVector<double>& xData,
			QVector<double>& yData, QString& errMsg) const;
		//复数图目标：x/yp/ym 三向量装配，产出 MAG/PHA 成对数据（命名约定配对）
		bool buildFilePairData(const DefineCurveEntry& e, QVector<double>& xData,
			QVector<double>& yMag, QVector<double>& yPha, QString& errMsg) const;
		bool buildMathData(const DefineCurveEntry& e, QVector<double>& xData,
			QVector<double>& yData, QString& errMsg, int maxSamples = -1) const;
		bool buildMathPairData(const DefineCurveEntry& e, QVector<double>& xData,
			QVector<double>& yMag, QVector<double>& yPha, QString& errMsg) const;
		bool buildValueData(const DefineCurveEntry& e, QVector<double>& xData,
			QVector<double>& yData, QString& errMsg) const;
		//复数图目标（数值来源）：表格 X/Yp/Ym 三列装配为成对数据（Ym=上面板幅值，
		//Yp=下面板相位；X 空格按 0 兜底）
		bool buildValuePairData(const DefineCurveEntry& e, QVector<double>& xData,
			QVector<double>& yMag, QVector<double>& yPha, QString& errMsg) const;
		//--- 混合来源（按向量侧）装配：各侧按自身来源装配取值序列，按行位配对、
		//--- 行数取较少者截断（用户拍板）；全数值/全数学保持既有装配口径
		bool assembleSideData(const DefineCurveEntry& e, int vector, QVector<double>& out,
			QString& errMsg) const;   //单侧按该侧来源装配取值序列
		bool evalMathSide(const DefineCurveEntry& e, const QString& expr, QVector<double>& xs,
			QVector<double>& out, QString& errMsg) const;   //数学表达式按采样求值（xs 入/出=实际采样）
		bool buildMixedXYData(const DefineCurveEntry& e, QVector<double>& xData,
			QVector<double>& yData, QString& errMsg, int maxSamples = -1) const;
		//maxSamples>1 时 X 侧采样先均匀抽点再求值（buildMathData 预览同款，
		//供预览与实际应用同源）
		bool buildMixedPairData(const DefineCurveEntry& e, QVector<double>& xData,
			QVector<double>& yMag, QVector<double>& yPha, QString& errMsg) const;
		//切入"数值"（按侧）：把该侧切出前来源内容装配进该侧数值串；该侧旧来源
		//无内容时保留已存数值串（数值可能是该侧唯一内容），有内容装配不出才清空
		bool dumpSideToValueText(DefineCurveEntry& e, int vector) const;

	private slots:
		void onListSelectionChanged();
		void onAddClicked();
		void onCutClicked();
		void onCopyClicked();
		void onPasteClicked();
		void onNameCheckToggled(bool checked);
		void onNameEdited(const QString& text);
		void onVectorChanged(int id);          //向量单选组（0 X 1 Y）
		void onSourceChanged(int id);          //来源单选组（0 文件 1 数学运算 2 数值）
		//文件页联动
		void onFileComboChanged(int index);
		void onSubcaseRowChanged(int row);
		void onSchemaRowChanged(int row);
		void onObjectRowChanged(int row);
		void onComponentRowChanged(int row);
		//数学页
		void onXEdited(const QString& text);
		void onExprEdited(const QString& text);
		void onYEdited(const QString& text);   //Y 向量框编辑（数学来源=表达式同源）
		void onYmEdited(const QString& text);  //复数图幅值框编辑（数学来源=表达式同源）
		void onExprClearClicked();
		void onSolveToggled(bool checked);
		void onFuncSearchChanged(const QString& text);
		void onFuncCategoryChanged(int index);
		void onFuncTreeSelectionChanged();
		void onObjTreeSelectionChanged();
		void onInsertDirectClicked();
		void onInsertToExprClicked();
		//数值页
		void onValueCellChanged(int row, int col);
		void onAddValueRow();
		void onDeleteValueRow();
		void onClearValueRows();                 //清空数值表全部行（设计稿工具条第三钮）
		void onValueContextMenu(const QPoint& pos);  //数值表右键菜单（插入/剪切/复制/粘贴/删除）
	//生命周期
	void onImportFile();
	void onFileAdded(const QString& filePath);
	void onNodeAboutToBeRemoved(int nodeId);   //目标图表/祖先页被删 -> 退出模式关闭
	void onDocStructureChanged(int nodeId);    //文档结构变化 -> 重建曲线引用树
	void onCurvePicked(int chartId, int curveNodeId);  //视窗 Shift+左键拾取曲线
	void onDefineStarted(int chartId);         //定义曲线模式开启：重绑目标并弹出
	void onApply();

	protected:
		//标题栏按下开始拖动 / 拖动中移动（照 mApplyStyleDlg）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;
		//右下角缩放手柄贴角跟随（手柄不占布局行，对齐 mApplyStyleDlg）
		void resizeEvent(QResizeEvent* event) override;
		//向量框点击 -> 切换曲线拾取目标侧（数学运算来源默认激活拾取）
		bool eventFilter(QObject* watched, QEvent* event) override;

	private:
		static mDefineCurveDlg* _instance;   //单例

		int _targetChartId = -1;        //目标图表节点id（打开时绑定）
		int _lastChartType = -1;        //上次"整条↔按侧字段互同步"时的目标图表类型
		                                //(-1=未同步；弹窗存活期间经参数栏切图表类型时
		                                //据此判定条目字段以哪套为准，见 ctor 联动)
		bool _needCenter = true;        //无已存几何时首次打开居中于父窗口
		bool _syncing = false;          //程序回填中：输入侧信号不写回条目

		QList<DefineCurveEntry> _entries;   //待新增条目（行序即列表序）
		int _current = -1;                  //当前选中条目下标（列表当前行）
		int _counter = 0;                   //"曲线 N"编号（会话内自增）
		DefineCurveEntry _clipEntry;        //剪切/复制粘贴板
		bool _hasClip = false;
		QString _currentFuncName;           //参数表当前函数（空=未选）
		int _pickingVector = -1;            //曲线拾取目标侧：0 X 1 Y/yp 2 ym -1 未
		                                    //激活（数学来源默认激活并随激活向量
		                                    //侧跟随，点击输入框仅切换目标侧；到
		                                    //视窗 Shift+左键点曲线回填——X 侧整体
		                                    //替换，Y/yp/ym 侧光标处插入）

		QWidget* _titleBar = nullptr;   //标题栏（拖动热区）
		QSizeGrip* _grip = nullptr;     //右下角缩放手柄（覆盖贴角，不占布局行）
		QPoint _dragOffset;             //拖动起始偏移
		QPoint _pressPos;               //标题栏按下点（拖动阈值起算，防双击微动挪窗）
		bool _dragging = false;         //拖动中标志

		//左栏
		QCheckBox* _nameCheck = nullptr;         //曲线名称复选框
		QLineEdit* _nameEdit = nullptr;          //曲线名称
		QListWidget* _curveList = nullptr;       //待新增曲线列表
		QPushButton* _cutBtn = nullptr;          //剪切
		QPushButton* _copyBtn = nullptr;         //复制
		QPushButton* _pasteBtn = nullptr;        //粘贴
		QPushButton* _addBtn = nullptr;          //新增
		QPushButton* _addPmBtn = nullptr;        //新增 P/M（复数图目标显示）
		QPushButton* _addRiBtn = nullptr;        //新增 R/I（复数图目标显示）

		//右栏
		QWidget* _rightPanel = nullptr;          //右栏整体（无条目时禁用）
		QRadioButton* _xRadio = nullptr;         //向量 X
		QRadioButton* _yRadio = nullptr;         //向量 Y（柱状图数学来源时文案改为"表达式"，复数图改为"yp"）
		QRadioButton* _ymRadio = nullptr;        //复数图幅值侧 ym（仅复数图目标显示）
		QWidget* _vectorSection = nullptr;       //向量区容器（标题+X/Y 行，数值来源隐藏）
		QWidget* _xRowWidget = nullptr;          //向量 X 行容器（柱状图整体隐藏）
		QWidget* _yRowWidget = nullptr;          //向量 Y/表达式 行容器
		QWidget* _ymRowWidget = nullptr;         //复数图 ym 行容器（仅复数图目标显示）
		QButtonGroup* _vectorGroup = nullptr;
		QLineEdit* _xEdit = nullptr;             //X 回显（仅数学运算来源可编辑引用）
		QLineEdit* _yEdit = nullptr;             //Y 向量（数学运算且 Y 激活时可编辑=表达式；其余置灰回显）
		QLineEdit* _ymEdit = nullptr;            //复数图幅值侧（数学来源可编辑表达式，文件/数值来源置灰回显）
		QRadioButton* _fileRadio = nullptr;      //来源：文件
		QRadioButton* _mathRadio = nullptr;      //来源：数学运算
		QRadioButton* _valueRadio = nullptr;     //来源：数值
		QButtonGroup* _sourceGroup = nullptr;
		QStackedWidget* _sourceStack = nullptr;  //三张来源页

		//文件页
		QComboBox* _fileCombo = nullptr;         //数据文件（全路径下拉）
		QToolButton* _importBtn = nullptr;       //导入（文件夹图标按钮）
		QListWidget* _subcaseList = nullptr;     //工况
		QWidget* _subcaseColumn = nullptr;       //工况列容器（ascii 数据集整列隐藏，R22 三级化）
		QListWidget* _schemaList = nullptr;      //结果类型
		QListWidget* _objectList = nullptr;      //对象
		QListWidget* _componentList = nullptr;   //分量
		QLabel* _subcaseCount = nullptr;         //工况列计数（1/N）
		QLabel* _schemaCount = nullptr;          //结果类型列计数
		QLabel* _objectCount = nullptr;          //对象列计数
		QLabel* _componentCount = nullptr;       //分量列计数

		//数学页
		QTabWidget* _builderTabs = nullptr;      //函数 / 曲线
		QLineEdit* _funcSearch = nullptr;        //函数搜索
		QComboBox* _funcCategory = nullptr;      //函数分类
		QTreeWidget* _funcTree = nullptr;        //函数树
		QTreeWidget* _objTree = nullptr;         //曲线引用树
		QTableWidget* _funcParamTable = nullptr; //名称/值参数表
		QPushButton* _directInsertBtn = nullptr; //直接插入（当前向量）
		QPushButton* _useExprBtn = nullptr;      //用于表达式
		QLineEdit* _exprEdit = nullptr;          //表达式
		QToolButton* _exprClearBtn = nullptr;    //清空表达式
		QCheckBox* _solveCheck = nullptr;        //计算求解
		QTableWidget* _previewTable = nullptr;   //值预览

		//数值页
		QTableWidget* _valueTable = nullptr;     //数值序列表（Index/Categories/曲线名列；
		                                //柱状图目标为共享表：Categories 一列 + 每条目一列）
		QToolButton* _addValueRowBtn = nullptr;  //新增行
		QToolButton* _delValueRowBtn = nullptr;  //删除选中行
		QToolButton* _clearValueBtn = nullptr;   //清空全部行（TODO：图标后续提供，到位后 setIcon）
		QList<QStringList> _valueClipRows;       //数值行剪贴板（每行=[刻度名, 各曲线列值...]，
			                                //常规表 2 元素/共享表 1+N 元素；右键剪切/复制放入、
			                                //粘贴取出；弹窗会话内有效）
		QList<QStringList> _sideEchoTokens;      //非数值侧列的只读回显 token 缓存（下标=向量侧
			                                //0/1/2，'f' 6；数值侧列为空表）——切表时现算缓存，
			                                //行插入后只读列按行位回显免重复装配

		//底部
		QPushButton* _closeBtn = nullptr;        //关闭
		QPushButton* _applyBtn = nullptr;        //应用
	};
}
