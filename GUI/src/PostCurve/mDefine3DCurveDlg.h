#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - "定义曲线"对话框（三维曲线，功能弹窗，单例）
//
// 入口："3D曲线"Ribbon页签-文件组-"定义曲线"按钮（mPostMenuContributor::
// build3DCurvePage，在"新建"旁，参考图 design/3d-curve/ref-ribbon-3dcurve-tab.png）。
// 弹窗形态照截图三栏布局：
//   左栏  对象行（"对象"复选框 + 名称编辑框 + 类型视图下拉[曲线/曲面/瀑布图/
//         柱状图]）+ 待新增对象列表 + 剪切/复制/粘贴/新增——"对象"复选框=该
//         条目参与应用（取消勾选仅跳过不删除）；类型下拉=类型视图过滤器：
//         列表只显示该类型条目（其他类型隐藏不删除、可切回），不回写条目
//         类型；打开即把目标三维图内全部曲线载入为条目（树序，编辑绑定随
//         节点；打开前选中曲线/其 x/y/z 轴壳为初始选中条目，类型视图随之
//         切换保证可见；无三维图/无曲线=初始空态，仅"新增/关闭"可用），
//         "新增"/"粘贴"条目类型跟随当前视图类型（粘贴跨类型自动切视图）；
//         列表选中条目 -> 右栏编辑其定义；
//   矢量列  X/Y/Z/U/V 单选（活动向量侧，右栏各页随其联动）；
//   来源列  文件/数学运算/数值 单选——切换"活动向量侧"的来源并切页：
//     文件     文件行（浏览... + 路径只读回显，txt/csv）+ 类型（数值表）+
//              请求（当前数据表）+ 分量（解析出的列按向量侧独立选择，X/Y/Z
//              各自取文件的一列，未选分量=定义不完整；矩阵表文件列名展示恒
//              为 Column N[N=数据列序]，不展示数值 token 表头；弹出列表高度
//              上限 10 行，超出滚动）；
//     数学运算 表达式编辑框（活动向量侧各自持有）+ 计算器键盘（数字/运算符/
//              函数按键光标处插入；自变量 x：X 侧=参数 u∈[0,1] 均匀 101 点，
//              Y/Z 侧=该条目已装配的 X 数据；自变量 y：四类型 Z 侧均开放=本
//              对象 Y 数据（键盘 y 键动态启停；曲面/瀑布图/柱状图按 X×Y 全
//              网格求值，曲线按行位逐点配对求值）；向量级函数（积分/微分/
//              FFT/PSD/滤波等）与范围/子对话框类按键本期未支持，置灰带 tooltip）；
//     数值     逐行输入表 点|U|X|Y|Z（点=只读行号；U 列恒可编辑且空白恒归一
//              为 "0"——仅 U 补 0，X/Y/Z 空格=装配跳行语义保留；X/Y/Z 列
//              仅当该侧来源=数值时可编辑[网格态例外：网格类条目 Z=数值时三列
//              全部可编辑、无只读回显]，其余侧列只读回显该侧当前来源装配
//              数据['f' 6 行位对齐，超出留空；Z 侧数学来源按行位逐点配对
//              二元求值 f(X[i],Y[i])，非有限行留空]）+ 表上方 [+][-][清空]
//              行操作工具条 + 表右键菜单（在上方/下方插入一行、剪切、复制、
//              粘贴、删除）；来源切到"数值"时把该侧切出前来源内容装配进该
//              侧数值列（文件=已解析+已选分量列整列；数学=条带采样口径求值：
//              X/U 侧 u∈[0,1] 101 点、Y 侧以装配的 X 数据为采样；Z 侧曲线类
//              按行位逐点配对二元求值——与曲线逐点应用口径同源，含 y 表达式
//              可求值，网格类条目=整网格摊平：X/Y/Z 逐单元 rows×cols 行、
//              维度落条目 valueGrid（曲面从数值可绘制；切回"数学"来源应用
//              仍走网格链路）；非有限单元写空串；旧来源无内容=保留已存数值
//              列，有内容装配不出=清空该侧列）；该侧数值列被用户改过（改格/
//              行级操作置脏，程序回填不算）后离开"数值"弹确认框，取消=单选
//              回退状态零变化——整体对齐二维 mDefineCurveDlg 数值页
//              （applySourceChange/dumpSideToValueText 链路同构，
//              见 buildValuePage/applySourceChange）；
//   底部  关闭 / 应用。
//
// 应用（onApply）：逐条目把 X/Y/Z 三侧按各自来源装配。网格态前置分支（网格类
// 条目持网格维度且 Z 侧来源=数值）：数值表 X/Y/Z 三列按转换落定的维度直读装配
// rows/cols 入档（表即数据；行数≠维度=退条带+提示）。条带模式（其余一切组合）：
// 等长取值序列（按行位截齐取较少者，模块既有口径；X 侧数学来源按 u∈[0,1] 采样
// 101 点，Y/Z 侧数学表达式经 mCurveExpressionEvaluator 以 X 数据为采样横坐标求
// 值）；网格模式（网格类条目 plot3dType∈{0,2,3}——瀑布图/曲面/柱状图——且 Z 侧
// 来源=数学运算）：X/Y 两轴独立装配后按 X×Y 笛卡尔积全网格求值 Z（行=Y 列=X），
// 三轴逐单元扁平展开入档；逐点配对模式（曲面条目 plot3dType==1 且 Z 侧来源=数学
// 运算）：X/Y 装配后按行位配对截齐到较少者，Z 逐点 z_i=f(x_i,y_i)，条带入档
// rows/cols 恒 0。
// 失败红字报到信息输出（带条目名）不写文档；全部成功 -> 定位/新建三维图表（R30
// 单视口口径同 mCreate3DPlotDlg：全文档查 chartType=5，无则新建宏 addPage→
// add3DCurve）后条目绑定生成曲线（editCurveId 对齐二维口径）：绑定节点存活=
// 复用原位更新（数据 set3DSeriesData 直写宏外不进撤销栈；条目名≠节点名时名称
// 命令随宏可撤销），无绑定/绑定已失效（曲线被删）=宏内 add3DCurve 新建并回写
// 绑定（一次撤销整批还原新建；plot3dType=条目自身类型；网格模式 rows/cols=
// len(Y)/len(X) 实值入档渲染真网格曲面，条带模式 rows/cols=0——按侧选列装配
// 无网格形状，曲面/瀑布按所选列装配为条带的诚实呈现。装配全部勾选条目，含当
// 前过滤视图外的隐藏类型）。数据落档后溯源落模（set3DCurveProvenance：主标记
// 数学>文件>数值 + 三侧表达式/文件分量列下标按侧独立写入）——重开弹窗载入时
// 按侧还原定义（表达式在=数学侧还原，分量列在=文件侧还原，缺席侧=当前数据倒
// 数值列兜底，网格维度随 plot3dRows/Cols 还原数值网格态）。成功提示区分新建/
// 更新。成功后条目留存，可继续修改再次应用（原位更新不重复新建；粘贴条目解除
// 绑定）。
//
// 边界（本期不做）：U/V 侧定义可编辑但不参与装配与校验（参数曲面定义后续接
// 入）；既有曲线载入按溯源还原定义（数学表达式/文件分量列在=按侧还原，缺席
// 侧=当前数据倒数值列兜底——本弹窗落模曲线完整还原；"从文件新建三维图"等
// 无溯源曲线=数值倒表态载入，应用原位更新绑定点）；矩阵文件整表装配（任何类型
// 均按所选列逐侧装配——网格类（瀑布/曲面/柱状）Z 侧数学来源已按 X×Y 全网格
// 装配（曲面真网格/瀑布矩阵瀑布/柱状柱阵）、曲线 Z 侧数学来源逐点配对条带
// （20261113 修订三二次拍板四类型扩展），文件/数值来源仍为条带；整表网格口径
// 用"从文件新建三维图"弹窗）；
// 数学页 范围/拟合/滤波/曲线/函数 等子对话框按键；pch 结果文件来源（请求/分量
// 当前固定"当前数据表"口径，即所选 txt/csv 的数据表）。
//
// 形态照 mCreate3DPlotDlg/mDefineCurveDlg(2D)：无边框白卡片（M徽标标题栏+✕，
// 可拖动缩放），非模态单例（showInstance 唤回 raise/activate），
// DlgGeom::bind 记忆大小/位置；右栏页面设最小宽度防线（窄宽下文件页控件不被
// 裁成只剩行标签的"光杆"态）。
// =============================================================================

#include "PostCurveGlobal.h"
#include "mCurveAsciiReader.h"

//QT
#include <QList>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QEvent;
class QGridLayout;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QMouseEvent;
class QPushButton;
class QRadioButton;
class QResizeEvent;
class QSizeGrip;
class QStackedWidget;
class QTableWidget;
class QToolButton;

namespace PostCurve
{
	//向量侧（矢量单选序；sides 下标即此序）
	enum Define3DVector
	{
		kVecX = 0, kVecY, kVecZ, kVecU, kVecV, kVecCount
	};

	//单侧定义（来源按向量侧独立，照 2D 定义曲线的 per-vector-source 模型）
	struct Define3DSide
	{
		int source = 0;         //0 文件 1 数学运算 2 数值
		int component = -1;     //文件来源：分量列下标（AsciiColumnFile.columns 下标，-1=未选）
		QString expression;     //数学来源：表达式（自变量 x）
	};

	//定义中的三维曲线对象（对话框会话内的一条待新增条目；应用前只存在于本对话框）
	struct Define3DEntry
	{
		bool include = true;    //"对象"复选框：勾选=应用时写入文档
		QString name;           //对象名（新增/粘贴时按条目类型自动"曲线/曲面/瀑布图/
		                        //柱状图 N"，可编辑）
		int plot3dType = 1;     //0 瀑布/1 曲线/2 曲面/3 柱状（创建时取当前类型视图
		                        //映射，创建后固定，同 set3DSeriesData）
		Define3DSide sides[kVecCount];   //X/Y/Z/U/V 各侧来源定义
		QString filePath;                //文件来源：数据文件（全条目共享一份，
		                                 //分量列按侧独立选择——照截图单文件行）
		bool fileParsed = false;         //filePath 已解析进 parsed（失败置 invalidFile）
		bool fileInvalid = false;        //上次解析失败（避免反复重读；换文件时复位）
		AsciiColumnFile parsed;          //解析缓存（浏览/应用时按需填充）
		QList<QStringList> valueRows;    //数值来源整表内容（每行=[U, X, Y, Z] 四值，
		                                 //点列=行号自动；首访自动补 8 行零值）
		bool valueSeeded = false;        //8 行零值种子已补（首访种子只补一次——
		                                 //清空后的空表是合法持久态，切条目/切侧
		                                 //回显不得重新播种，二维清空同口径）
		bool xValueDirty = false;        //X 侧数值列内容被用户改过（改格/行级
		                                 //操作置脏；程序装配/回填/回显不算改动）
		                                 //——离开"数值"脏确认按侧判定，2D 按侧
		                                 //脏模型同口径
		bool yValueDirty = false;        //Y 侧数值列被用户改过（同上）
		bool zValueDirty = false;        //Z 侧数值列被用户改过（同上）
		bool uValueDirty = false;        //U 侧数值列被用户改过（同上；U 列恒
		                                 //可编辑，仅 U 侧来源=数值时计脏）

		//数值网格态：网格类条目（瀑布/曲面/柱状）Z 侧数学切"数值"时整网格摊平
		//进表（X/Y/Z 逐单元 rows×cols 行）——维度随转换落定，应用按维度直读
		//装配不回猜；表行数与维度不符（增删行后）=退条带+提示。Z 侧切离"数值"
		//退网格态（表内容保留不回滚）；曲线类恒 false（逐点配对转换零变化），
		//条目类型创建后固定，无"切类型"入口（判定自带类型条件兜底）
		bool valueGrid = false;          //网格态有效标志（Z 侧来源=数值时才有意义）
		int valueGridRows = 0;           //摊平网格行数（=Y 轴单元数）
		int valueGridCols = 0;           //摊平网格列数（=X 轴单元数）

		//编辑模式（对齐 2D mDefineCurveDlg editCurveId 同名语义）：>=0 = 绑定的
		//已生成三维曲线节点 id，应用时节点存活即原位更新（不新建；曲线被删=
		//重建重绑）；-1 = 新建条目（粘贴条目自动解除绑定）
		int editCurveId = -1;

		QString valueAt(int row, int dataCol) const;   //dataCol 0=U 1=X 2=Y 3=Z
		void setValueAt(int row, int dataCol, const QString& text);
	};

	class POSTCURVE_EXPORT mDefine3DCurveDlg : public QWidget
	{
		Q_OBJECT
	public:
		//单例弹窗唤回（已打开则 raise/activateWindow 置前）
		static void showInstance(QWidget* parent = nullptr);

		~mDefine3DCurveDlg() override;

		mDefine3DCurveDlg(const mDefine3DCurveDlg&) = delete;
		mDefine3DCurveDlg& operator=(const mDefine3DCurveDlg&) = delete;

	private:
		explicit mDefine3DCurveDlg(QWidget* parent);

		//--- UI 构建 ---
		void buildUi();
		QWidget* makeVecSrcSeparator(); //列间竖分隔线
		QWidget* buildListPanel();      //左栏：对象行 + 列表 + 条目按钮
		QWidget* buildVectorPanel();    //矢量列（X/Y/Z/U/V 单选）
		QWidget* buildSourcePanel();    //来源列（文件/数学运算/数值 单选）
		QWidget* buildRightPages();     //右栏：三张来源页（堆叠）
		QWidget* buildFilePage();       //来源页：文件
		QWidget* buildMathPage();       //来源页：数学运算（表达式 + 键盘）
		QWidget* buildValuePage();      //来源页：数值（工具条 + 表格 + 右键菜单）
		QPushButton* addKeypadButton(QGridLayout* grid, int row, int col, int spanRows,
			int spanCols, const QString& label, const QString& insertText,
			bool enabled, const QString& tooltip);   //返回所建按键（y 键须存成员指针
			                                         //做动态启停，其余调用点忽略返回值）
		void bindComboPopupHeight(QComboBox* box);   //下拉弹出视图挂高度钳制过滤器
		                                             //（B04，机制见 eventFilter）

		//--- 条目与面板同步 ---
		QString nextEntryName(int plot3dType) const;      //新增默认名按条目类型"前缀 N"
		                                                  //（各类型会话内独立自增）
		int currentEntryIndex() const;                    //当前选中条目下标（无选中 -1）
		//--- 类型视图过滤（列表行 <-> 条目下标经 UserRole 映射；_current 存条目下标）---
		int viewPlot3dType() const;                       //当前类型视图的 plot3dType（下拉序映射）
		void rebuildFilteredList();                       //按当前类型视图重建过滤列表
		int entryAtRow(int row) const;                    //过滤列表行 -> 条目下标（无效 -1）
		int rowForEntry(int entryIdx) const;              //条目下标 -> 过滤列表行（不在视图 -1）
		void loadEntryToPanel();                          //条目 -> 左栏对象行/右栏当前侧
		void loadSideToPanel();                           //活动向量侧 -> 来源单选/页/分量/表达式

		//--- 分量列展示名（仅展示层改名；分量下标恒=数据列下标，映射零变化）---
		//矩阵表文件恒返回 Column N[N=col+1]（不展示数值 token 表头，与曲线部分
		//口径一致）；其余=解析缓存现行列名（表头名或 Column N 回退名）
		QString componentDisplayName(const Define3DEntry& e, int col) const;
		void refreshValueTable();                         //数值表按当前条目整表重建（回显
		                                                  //token 现算；可编辑列随侧来源）
		void rebuildValueTableView();                     //按既有回显 token 缓存整表重建
		                                                  //（行插删路径复用缓存免重复装配；
		                                                  //U 列空白归一 "0"——仅 U 补 0）
		void computeSideEchoTokens(const Define3DEntry& e);   //非数值侧列回显 token 现算
		                                                  //缓存（'f' 6 行位对齐，2D 同名
		                                                  //思路）
		void refreshValueCellStates(int row);             //行内格只读性/回显/红字收口
		void ensureValueRows(Define3DEntry& e);           //数值表首访补 8 行零值
		void refreshValueToolBarStates();                 //数值页工具条启停（[-]随选中行、
		                                                  //[清空]随表行数，二维同款）
		void insertValueRowAt(int at);                    //在 at 位插一空白行（全 0，表尾
		                                                  //追加=at 越界钳制；新行落 U 格
		                                                  //直接可输）——工具条[+]与右键
		                                                  //插入共用
		void refreshActionStates();                       //按钮/右栏整体启停
		void refreshYKeyState();                          //数学页 y 键动态启停（四类型
		                                                  //Z 侧均开放自变量 y；UI 态
		                                                  //不写条目）

		//--- 来源切换转换链/按侧脏标记（对齐 2D mDefineCurveDlg 同名链路）---
		bool sideValueDirty(const Define3DEntry& e, int vector) const;    //按侧数值脏读
		                                                  //（V 侧无数值列恒非脏）
		void setSideValueDirty(Define3DEntry& e, int vector, bool dirty) const;   //按侧
		                                                  //数值脏写
		bool sideHasContent(const Define3DEntry& e, int vector) const;    //该侧当前来源
		                                                  //有内容（文件=已解析+分量在
		                                                  //界内；数学=表达式非空）
		bool leaveValueConfirmed();                       //"离开数值"脏确认框（2D 同
		                                                  //文案/按钮/默认钮；false=取消）
		bool applySourceChange(int id);                   //来源切换链路（活动向量侧）：
		                                                  //脏确认→倒表→落值→切页回显；
		                                                  //false=用户取消（单选已回退）
		bool dumpSideToValueText(Define3DEntry& e, int vector) const;     //切出前来源内容
		                                                  //装配进该侧数值列（须在该侧
		                                                  //source 落新值之前调用）
		bool zMathPointwiseEval(const Define3DEntry& e, QVector<double>& out) const;
		                                                  //Z 侧数学来源逐点配对二元
		                                                  //求值（只读回显共用，与
		                                                  //assembleEntry 曲线
		                                                  //逐点模式同源序列）：装配 X
		                                                  //（数学=u 101 点）→装配 Y
		                                                  //（采样横坐标=装配 X）→按行
		                                                  //位配对截齐到较少者（静默）
		                                                  //→evaluatePointwiseXY 逐点
		                                                  //z_i=f(x_i,y_i)；单变量表达
		                                                  //式为等价形态统一走此路径。
		                                                  //数值表是点表——各类条目
		                                                  //Z 侧数学只读回显同为逐点
		                                                  //口径不摊平网格（网格类转换
		                                                  //已改整网格摊平见
		                                                  //dumpSideToValueText；切回
		                                                  //"数学"应用仍走网格链路
		                                                  //evaluateGrid）。
		                                                  //成功=true 且 out=逐点值；
		                                                  //X/Y 装配不出/求值失败=
		                                                  //false 且 out 清空
		void markValueRowOpDirty(Define3DEntry& e);       //行级操作置脏全部数值侧
		                                                  //（复制不置脏，2D 调用点口径）

		//--- 装配与应用 ---
		//单侧装配取值序列：file=分量列（非法单元按 0 兜底带回计数）；
		//math=表达式按 xSamples 采样求值（X 侧传 u 均匀采样）；value=数值表列解析。
		//失败返回 false 并置 *err 带原因
		bool assembleSideVector(const Define3DEntry& e, int vector,
			const QVector<double>& xSamples, QVector<double>& out,
			QString* zeroNote, QString& err) const;
		//整条目装配 X/Y/Z。网格态前置分支（网格类条目持网格维度且 Z 侧来源=
		//数值）：数值表 X/Y/Z 三列即整网格摊平数据，按转换时落定的维度直读
		//装配 rows/cols（表即数据：不读各侧来源、不截齐；表行数≠维度=清条目
		//网格态走下方条带路径+note 提示，故 e 为非 const）。网格模式（网格类
		//条目 plot3dType∈{0,2,3}——瀑布图/
		//曲面/柱状图——且 Z 侧来源=数学运算）：X/Y 两轴独立装配后按 X×Y 笛卡尔
		//积全网格求值 Z，rows=len(Y)/cols=len(X) 带出，不做等长截齐（网格两轴
		//独立）；逐点配对模式（曲线 plot3dType==1 且 Z 侧来源=数学运算）：X/Y
		//装配后按行位配对截齐到较少者，Z 逐点 z_i=f(x_i,y_i)，rows/cols=0,0
		//条带入档；条带模式（其余一切组合）：既有逐侧装配+等长截齐口径，
		//rows/cols=0,0
		bool assembleEntry(Define3DEntry& e, QVector<double>& xs,
			QVector<double>& ys, QVector<double>& zs, int& rows, int& cols,
			QString* note, QString& err) const;
		//三维图表定位（全文档查 chartType=5，取首个命中；-1=无）
		int findExisting3DChart() const;
		//打开载入目标三维图（激活图表为三维图优先——多三维图会话跟随用户当前
		//视口；否则全文档首个，同 onApply 落点口径；-1=无）
		int target3DChartId() const;
		//打开载入（对齐 2D mDefineCurveDlg onDefineStarted 编辑模式）：先复位
		//（未应用定义直接丢弃），目标三维图内全部曲线按树序载入为条目
		//（addEntryFromCurve，编辑绑定随节点），打开前选中曲线/其 x/y/z 轴壳为
		//初始选中条目、类型视图随之切换；无三维图/无曲线=初始空态
		void loadExistingCurves();
		//既有曲线载入为单条目（对齐 2D addEntryFromCurve 同名语义）：名称/
		//plot3dType/编辑绑定随节点；逐侧按溯源还原定义——数学表达式在（X/Y=
		//xRef/yRef.expression，Z=z3DExpression）=数学侧，文件分量列下标在且
		//解析界内=文件侧，缺席侧=数值兜底（当前数据倒数值列）；只落 _entries
		//不碰列表 UI（载入批处理由 loadExistingCurves 收口重建过滤列表）
		void addEntryFromCurve(int curveId);
		//曲线当前数据倒条目数值列（2D dumpCurveValuesToEntry 同名同思路）：
		//三轴逐行 [U=0, X, Y, Z] 'f' 6（非有限写空串=装配跳行），仅数值兜底侧
		//列写值（其余列留空走只读回显）；网格类条目持有效网格维度（rows*cols==
		//点数）时落数值网格态（valueGridRows/Cols=plot3dRows/Cols）。倒表后
		//valueSeeded=true（不再种子补行）+ 按侧脏标记复位（程序倒表不算改动）
		void dumpCurveValuesToEntry(int curveId, Define3DEntry& e) const;
		void onApply();

	protected:
		//标题栏按下开始拖动 / 拖动中移动（照 mCreate3DPlotDlg）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;
		//右下角缩放手柄贴角跟随（对齐 2D 定义曲线弹窗）
		void resizeEvent(QResizeEvent* event) override;
		//下拉弹出高度钳制（B04）：本运行环境 setMaxVisibleItems 对弹出实际高度
		//不生效（含无 QSS 裸路径），改为弹出视图 Show 事件时按实测行高钳
		//maxHeight（上限=kComboMaxVisibleItems 行+弹出框余量；行数≤上限仅复位
		//钳制，短列表自然全展不被误钳）
		bool eventFilter(QObject* watched, QEvent* event) override;

	private slots:
		void onListSelectionChanged();
		void onAddClicked();
		void onCutClicked();
		void onCopyClicked();
		void onPasteClicked();
		void onIncludeToggled(bool checked);
		void onNameEdited(const QString& text);
		void onTypeChanged(int index);
		void onVectorChanged(int id);          //矢量单选组（0 X 1 Y 2 Z 3 U 4 V）
		void onSourceChanged(int id);          //来源单选组（0 文件 1 数学运算 2 数值；
		                                       //经 applySourceChange 链路：脏确认/
		                                       //倒表/切页回显）
		//文件页
		void onBrowseClicked();
		void onComponentChanged(int index);
		//数学页
		void onExprEdited(const QString& text);
		//数值页
		void onValueCellChanged(int row, int col);
		void onValueAddClicked();              //工具条[+]：表尾追加一空白行
		void onValueClearClicked();            //工具条[清空]：确认后清空全部行
		void onValueContextMenu(const QPoint& pos);   //表右键菜单（上/下方插入、
		                                              //剪切、复制、粘贴、删除）
		void onValueCutClicked();
		void onValueCopyClicked();
		void onValuePasteClicked();
		void onValueDeleteClicked();
		QList<int> selectedValueRowsImpl() const;   //数值表选中行号升序（按行选择）

	private:
		static mDefine3DCurveDlg* _instance;   //单例

		bool _needCenter = true;        //无已存几何时首次打开居中于父窗口
		bool _syncing = false;          //程序回填中：输入侧信号不写回条目

		QList<Define3DEntry> _entries;  //待新增条目（过滤列表项 UserRole 携带条目下标）
		int _current = -1;              //当前选中条目下标（过滤列表行经 UserRole 反查）
		int _vectorSide = kVecX;        //活动向量侧（矢量单选）
		int _typeCounters[4] = {};      //默认名"前缀 N"计数（下标=plot3dType 0..3，
		                                //各类型会话内独立自增、剪切/删除不回收）
		Define3DEntry _entryClip;       //条目剪贴板（剪切/复制放入，粘贴取出）
		bool _hasEntryClip = false;
		QList<QStringList> _valueClip;  //数值行剪贴板（每行=[U,X,Y,Z]，会话内有效）
		QList<QStringList> _sideEchoTokens;   //非数值侧列只读回显 token 缓存（下标=
		                                      //向量侧，'f' 6 行位对齐；整表重建时
		                                      //现算、行插删路径复用免重复装配，
		                                      //2D _sideEchoTokens 同思路）

		QWidget* _titleBar = nullptr;   //标题栏（拖动热区）
		QSizeGrip* _grip = nullptr;     //右下角缩放手柄（覆盖贴角，不占布局行）
		QPoint _dragOffset;             //拖动起始偏移
		QPoint _pressPos;               //标题栏按下点（拖动阈值起算，防双击微动挪窗）
		bool _dragging = false;         //拖动中标志

		//左栏
		QCheckBox* _includeCheck = nullptr;      //对象复选框（勾选=参与应用）
		QLineEdit* _nameEdit = nullptr;          //对象名
		QComboBox* _typeCombo = nullptr;         //类型视图（曲线/曲面/瀑布图/柱状图；
		                                         //驱动列表过滤与新增类型，恒可用）
		QListWidget* _curveList = nullptr;       //待新增对象列表（仅显当前类型视图条目）
		QPushButton* _cutBtn = nullptr;          //剪切
		QPushButton* _copyBtn = nullptr;         //复制
		QPushButton* _pasteBtn = nullptr;        //粘贴
		QPushButton* _addBtn = nullptr;          //新增

		//矢量/来源
		QRadioButton* _vectorRadios[kVecCount] = {};   //X/Y/Z/U/V
		QRadioButton* _fileRadio = nullptr;      //来源：文件
		QRadioButton* _mathRadio = nullptr;      //来源：数学运算
		QRadioButton* _valueRadio = nullptr;     //来源：数值

		//来源页
		QStackedWidget* _sourceStack = nullptr;  //三张来源页
		QWidget* _rightPanel = nullptr;          //右栏整体（无选中条目时禁用）

		//文件页
		QPushButton* _browseBtn = nullptr;       //浏览...
		QLineEdit* _fileEdit = nullptr;          //路径回显（只读，过长省略）
		QComboBox* _typeFixedCombo = nullptr;    //类型（固定"数值表"）
		QComboBox* _requestCombo = nullptr;      //请求（固定"当前数据表"）
		QComboBox* _componentCombo = nullptr;    //分量（按活动向量侧；首项"未选择"）
		                                         //弹出列表高度上限 10 行（超出行滚动）

		//数学页
		QLineEdit* _exprEdit = nullptr;          //表达式（活动向量侧）
		QPushButton* _yKeyBtn = nullptr;         //键盘"y"键（自变量 y：四类型 Z 侧
		                                         //均开放（网格类网格求值/曲线逐点
		                                         //配对），动态启停见 refreshYKeyState）

		//数值页
		QTableWidget* _valueTable = nullptr;     //点|U|X|Y|Z
		QToolButton* _vaddRowBtn = nullptr;      //[+] 新增一行数值（表尾追加）
		QToolButton* _vdelRowBtn = nullptr;      //[-] 删除选中行（随选中行启用）
		QToolButton* _vclearBtn = nullptr;       //[清空] 清空全部行（随表行数启用，
		                                         //点击弹确认框；文字钮待图标，
		                                         //ToolButtonTextBesideIcon 同二维）

		//底部
		QPushButton* _closeBtn = nullptr;        //关闭
		QPushButton* _applyBtn = nullptr;        //应用
	};
}
