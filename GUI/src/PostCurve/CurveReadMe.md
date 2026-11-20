# 曲线模块（PostCurve）架构说明

> 对应 UI 设计图：`F:\htx\ui设计\曲线\`（00~16）。
> 本文描述当前**框架骨架**的结构、三类界面的实现模式、以及后续功能的挂载点。

## 1. 架构总览

文档-视图 + 命令栈 + 属性页注册表。一份数据模型（CurveDocument），三个视图（曲线树 / 图表视口 / 属性页）订阅同一信号源；**所有修改经 QUndoCommand 命令栈**，天然获得撤销重做与三视图同步。

```
┌─ MainWindow（宿主，最小改动）───────────────────────────────┐
│  Ribbon"曲线"页(原NVH曲线页)  参数栏mInfoWindow  dock管理       │
│      │                            ▲ editCurveNode(nodeId)     │
│      │ connect（sig_requestNewViewport 等）                    │
└──────┼────────────────────────────┼──────────────────────────┘
┌──────▼────────────────────────────┴──────────────────────────┐
│  PostCurve（独立库，依赖方向 MainWindow → PostCurve，单向）      │
│                                                              │
│  CurveCore/       模型层：节点定义/CurveDocument/控制器          │
│  CurveCommands/   命令层：Add/Remove/SetProperty + QUndoStack │
│  CurveTree/       曲线树窗口(dock "CurveTree") + 类型筛选        │
│  CurveView/       mCurveChartView(PlotView封装) + 视口工厂       │
│  CurveWidgets/    折叠分组/颜色按钮/线型下拉（模块自带UI副本）      │
│  CurvePages/      属性页族 + 注册表 + 浮动属性编辑器              │
└──────────────────────────────────────────────────────────────┘
```

### 目录与文件

| 目录 | 文件 | 职责 |
|---|---|---|
| CurveCore | CurveDefine.h | CurveNodeType 枚举、各节点数据结构、CurveProp 属性名常量；节点类型词表（curveNodeTypeDisplayName/curveNodeDefaultName/nodeVisibleToggleProp 显隐属性键/nodeTypeRenameable 可重命名）；三维基准面（R41：DatumPlane3D/CurveDatumPlane3DData/图表级 datum3DVisible） |
| CurveCore | ChartTypeProfile.h/.cpp | **图表类型声明**（按图表类型分组的树内容与渲染行为，一张表收口）：一种 chartType 一份 profile——树内容节（固定骨架 fixedChildren[类型/默认名/显示名覆写/可删/默认可见]、用户节点分区 zoneOrder、右键菜单 menus、动态显示名 displayOverride）+ 渲染行为节（renderKind 画法分派/zeroLineStyle 零线样式/xTickSemantic X刻度语义，一期注册值=现状未消费零影响）；查表入口 `chartTypeProfile(chartType)` 未注册回退 XY(0)，`chartZoneRank` 分区序查询。消费点：createChart 骨架创建（nodeZoneRank 已删）、childInsertionRow 分区序、nodeDisplayText 复数图显示名覆写（特判已迁出）、mCurveTreeView 右键菜单查表组装（16-case 大 switch 已删）；详见 5.8 |
| CurveCore | mPointSeekCore.h/.cpp | 三维寻点核心（R47，纯静态工具类无 GUI 依赖可离屏单测）：三列步进序（极值=极值点集切换[局部峰∪谷下标升序，T47-2 20261117 对齐修正]/X=曲线类型按序号·其余升序稳定排序[T47-3]/Y 升序稳定排序）/极值点集 extremumPointSet/全局极值/局部峰谷（严格大于两邻、端点单侧、NaN 断口跳过）/恒值列判定/列启停类型分流（曲线类型 Y 列固定禁用，T47-2）/越界安全，mPointSeek3DPage 消费；对齐 HyperGraph 寻点行为，详见 5.13.1 R47 |
| CurveCore | CurveDocument.h/.cpp | QAbstractItemModel 树模型（roles: NodeTypeRole/NodeIdRole），单一数据源 |
| CurveCore | mCurveDocController.h/.cpp | **模块唯一入口**（单例）：命令式 API + QUndoStack + 选择中枢信号 |
| CurveCommands | CurveCommands.h/.cpp | CurveAddNodeCommand / CurveRemoveNodeCommand（快照撤销）/ CurveSetPropertyCommand |
| CurveTree | mCurveBrowserWindow.h/.cpp | 单例 dock 容器（key "CurveTree"），顶部节点类型筛选（设计图01.2） |
| CurveTree | mCurveTreeView.h/.cpp | 树视图：选中→选择中枢、右键菜单 |
| CurveView | mCurveChartView.h/.cpp | PlotView::mPlotView 封装，按模型渲染曲线/轴/图例/页眉/基准线 |
| CurveView | mCurveAxisStripWidget.h/.cpp | 副轴自绘条带（多坐标轴第2枚起：X3../Y3..，刻度/数字/标题，见 5.6；mCurveScaleDraw 'g'4 格式定义于此共用） |
| CurveView | mCurveViewportFactory.h/.cpp | 图表节点新增→自动开视口；删除（含撤销）→自动关视口；全局交互模式持有（二维注释/基准线/三维注释模式广播+新物化视图承接）；寻点标记状态 {_chartId,_curveId,_pointIndex}（R47）+setPointSeekMarker 转发对应 m3DChartView+新物化回放+AboutToBeRemoved 清除，详见 5.13.1 R47 |
| CurveView | m3DChartAnnotationItem.h/.cpp | 三维图画布注释项（R42 二期建；R46 扩牵引线形态，chartType=5）：每个可见注释一个 QWidget 子控件（宿主视图子控件非画布子控件——rebuild 重建画布不受影响；集合级重建 hide+deleteLater 防悬挂）叠画布之上；两形态按 kAttachMode 分流——0 内容锚定（R46 注释模式打点创建）=牵引线+注释框（框左上角=锚点像素+固定偏移 +35,-32 对齐 PlotView curveBoxOff，本项几何=框∪锚点包围盒，牵引线=锚点→框中心止于入框边、边框有效色 1.2px 局部抗锯齿）/1 窗口（存量口径）=框左上角即锚点像素；paintEvent 自绘 背景(kBgColor 无效→半透明白图例同款)/边框(kBorderShow 且 kBorderWidth>0；borderMode==1 且自定义色有效→自定义色否则默认深灰)/多行文本(kFont+kColor 无效→#333、kAlign 逐行水平对齐、模板求值文本、固定内边距 4px)；锚点=kPosX/kPosY/kPosZ 三维数据坐标按相机投影定位（随视角旋转/缩放每帧重投影，越界收敛、相机身后收场隐藏）、框内容自适应宽高；模板求值渲染期现算（{Label}→本名/{X}{Y}{Z}→注释自身锚点数据坐标 'g',6/{U}{V}{W} 与未知 token 字面）；交互=点击选中（setCurrentNodeId）+拖拽超 4px 实时跟随松开宏"移动注释"一次提交（**锚点像素**反投影过原锚点相机平行面回写三键——牵引线形态=项位置+项内锚点偏移）；详见 5.13.1 R42（含同日三维锚点修订）/R46 |
| CurveWidgets | mCurveFoldSection.h/.cpp | 折叠分组（外观对齐参数栏 FoldSection，模块自带副本） |
| CurveWidgets | mCurveEditorWidgets.h/.cpp | 颜色按钮 / 线型下拉 |
| CurveWidgets | mCurveChannelSelector.h/.cpp | 通道选择器共享控件：数据文件下拉(+“导入...”)+工况→结果类型→对象→分量四级联动（数据**现取现用**数据源仓库，不缓存 file()/index() 内部指针——addFile 可能使其失效）；值模型 ChannelSelection（id 形态五元组，fromModelTexts/subcaseText/objectText 与模型来源显示串互转，"Subcase %1"/"Point id %1" 取串尾数字反解）；内嵌与弹出（pickChannel：Qt::Popup 白卡片+确定/取消，选择完整才可确定，现建现销）两种宿主形态；R22 起 csv/txt 数据集（fileKind 分流）：ascii 三级联动（隐藏工况列，结果类型=[Unknown]/对象=[Block 1]/分量=列名），内嵌/弹出两形态同源生效 |
| CurvePages | mCurvePropPageBase.h/.cpp | 属性页基类：setNodeId→loadFromModel；控件→commit()→命令栈 |
| CurvePages | mCurvePropPages.h/.cpp | 参数栏页面族：页/图表/曲线/坐标轴/图例/页眉/页脚/基准线/注释 |
| CurvePages | mCurveEditorPages.h/.cpp | 弹窗专属页面族：曲线（03.1）/坐标轴（05~07）/注释（15）快捷编辑页 |
| CurvePages | mCurvePropPageManager.h/.cpp | NodeType→页面工厂**双角色注册表**（参数栏/弹窗）；参数栏缓存实例 + 弹窗独立实例（弹窗未注册类型回退参数栏页）；R33 起 takeParamBarPage 对 Curve 节点按父图表 chartType==5 分流三维专属页（负变体键独立缓存槽，见 5.13.1 R33），R34 同款方案扩 Annotation 节点（三维注释页，见 5.13.1 R34） |
| CurvePages | mPropertyEditorPopup.h/.cpp | 无边框浮动属性编辑器（750×400 可拖动缩放，白底标题栏+M徽标，设计图03.1） |
| CurvePages | mCurve3DPropPage.h/.cpp | 三维图曲线节点专属参数栏页（R33，chartType=5 的 Curve 子节点）：三组行集对齐设计图——标准[可见性(生效)]/曲线标签[6 行占位，标签内容置灰回显曲线本名]/外观与着色[**按 plot3dType 二分两套行集（R38）**：曲线类九行全生效（R36 七行翻活：颜色/线宽/线型/符号样式+色块/符号间隔/符号大小/线下区域填充/填充透明度/云图勾选+变量下拉），按 plot3dType==1 整组动态启停，"编辑图例..."钮恒占位；色带类行集按类型：曲面/瀑布四行=显示模式[仅曲面生效]/颜色/不透明度/云图、柱状仅颜色一行（20261113 勘正；除曲面显示模式外全置灰占位照设计稿截图）]；提交经 m3DChartView 保视角增量刷新生效；详见 5.13.1 R33/R36/R38 |
| CurvePages | mAnnotation3DPropPage.h/.cpp | 三维图注释节点专属参数栏页（R34 建、R42 二期翻活；**R46 去注释管理组**——创建入口改 Ribbon 按钮检出模式+画布内容打点，注释管理[显示注释/注释名下拉/注释列表/新增·剪切·复制·粘贴]整组退役：新增=画布打点、显隐=树勾选、导航=树选中/画布点注释项，chartType=5 的 Annotation 子节点）：三组 9 行——文字[文本编辑区(生效 kText，失焦且文本有变化才提交)/对齐(生效 kAlign)+(X)(Y)(Z)(U)(V)(W)(Label) 七钮一行——{X}{Y}{Z}{Label} 四钮生效 R42（光标处插 token 不直接 commit）、{U}{V}{W} 维持置灰占位]/属性[显示边框(生效 R42 kBorderShow)/文字颜色/字体设置/背景颜色/边框线宽度(生效 kColor/kFont/kBgColor/kBorderWidth)]/附着对象[附着方式(置灰回显"三维"——模型两档位 0=内容锚定带牵引线（R46 打点创建）/1=窗口（旧入口存量），均锚定三维不提供切换)]；注册按父图表 chartType==5 负键分流（弹窗族不注册）；详见 5.13.1 R34/R42/R46 |
| CurvePages | mWaterfall3DPage / mCurveStyle3DPage / mPointSeek3DPage / mSectionQuery3DPage / mDatum3DPage（各 .h/.cpp） | 三维图参数栏工具页五件（R37，3D曲线工作台 Ribbon 按钮点击挂载；**非节点绑定页**——不继承 mCurvePropPageBase、不进注册表，宿主 mPostMenuContributor 懒创建 QPointer 缓存经 IPanelHost setMainWidget 挂载）：瀑布图/数据查询两页=纯展示占位（R31 占位纪律：构造即禁用、不连 commit、值=设计稿截图值）；数值页=**R47 翻活**（寻点工具闭环：曲线下拉[当前激活三维图全部 Curve 子节点树序按 id 键定，随 sig_currentChartChanged/文档信号刷新保选择]+极值/X轴/Y轴三列导航[完整语义，算法单一实现 CurveCore/mPointSeekCore——步进序/全局极值/局部峰谷/恒值列整列灰]+画布橙点标记[工厂全局单标记转发，视图态]+坐标数据表四列只读['f',4，网格类行主序摊平]+当前行加粗主题蓝高亮+寻点行滚动联动（T47-4）；表格导出钮接线 CSV（表头三列无序号/'f',4/非有限空串/UTF-8 BOM，T47-4）、其余三钮置灰待后续单；详见 5.13.1 R47）；曲线样式页=图表级新页接既有功能（图形类型=类型视图过滤下拉，同 mDefine3DCurveDlg 口径 20261117 勘正、列表多选同调样式、勾选=kVisible、显示/隐藏=选中集批量宏、图例显示/隐藏=Legend kVisible、外观与着色两套行集按类型视图二分——曲线九行生效作用于选中集、色带类曲面/瀑布四行·柱状仅颜色占位，键集=mCurve3DPropPage R36/R38 口径）；基准页=**R41 翻活**（图表级工具页接基准面管理闭环：显示基准总开关 kDatum3DVisible+勾选列表 kVisible+新增/复制/删除宏+属性行九键生效，渲染触发=键集已在 m3DChartView 白名单零额外代码；宿主挂载链零改动——setChart 由页面自身经工厂 currentChartId 在构造/showEvent 刷新+sig_currentChartChanged 跟随，等效 mCurveStyle3DPage"挂载前刷新上下文"）；行集对齐 design/3d-curve/ 设计稿，详见 5.13.1 R37/R41/R47 |
| 模块根 | mDlgGeometry.h | 弹窗几何持久化（仅头文件）：`DlgGeom::bind(this, "DlgKey")` 一行接入——恢复上次大小/位置 + 隐藏时自动保存到 Settings.ini 的 [PostCurve] 组（详见文件头注释） |
| 模块根 | mCurveAsciiReader.h/.cpp | csv/txt 通用列数据纯解析器（R22，纯静态无 GUI 依赖，可独立编译单测）：首行判定表头（首个非空白行全 token 可解析 double=无表头，首行即数据行）；行内含逗号按逗号切分（保留空位）并剥成对包裹引号、否则按连续空白切分（逐行独立判定，以数据行为准）；列数=max(表头 token 数, 各数据行 token 数)；列名=表头 token 去引号去首尾空白，空位/缺位回退 "Column N"（1 基列序，同名列不去重、装配按首个匹配下标取列）；数据行非数值 token 记 NaN、行短补 NaN（保留行位），空白行跳过；编码去 UTF-8 BOM 后严格 UTF-8 可解码按 UTF-8、否则 Local8Bit（GBK 兼容）；成功判定 >=1 列且 >=1 行（err"文件无有效数据列/行"与 pch 文案同风格）。产出 AsciiColumnFile{columnNames/columns/rowCount} 经数据源仓库登记 |
| 模块根 | mCurveDataSourceStore.h/.cpp | 数据源仓库（单例）：会话内已导入数据集共享缓存（原始解析数据 + 层级索引 ListWgtStruct + 分量名规则），"从文件创建曲线"（mOpenCurveDlg）与"定义曲线"（mDefineCurveDlg）共用，新文件经 sig_fileAdded 通知订阅方；**R22 起双数据源**：`fileKind`（扩展名 .csv/.txt 不分大小写→AsciiColumn，其余→Pch）分发登记（addFile/addFileAsync 同步+异步两路，ascii 走 mCurveAsciiReader 登记为 AsciiColumnFile、与 PchFile 分存——`file()` 对 ascii 返回 nullptr 不伪造 pch 块，`contains`/`filePaths`/`clear` 两类一并覆盖）；ascii 索引固定骨架：xresulttype 空集（无 Time/Frequency 横坐标语义）、工况 1 / 结果类型 "Unknown" / 对象 "Block 1"（内部 id 恒 1，语义常量 asciiSchemaDisplayText/asciiSubcaseId/asciiPointId/asciiObjectDisplayText）、分量叶子 NumericFmtInfo{"ASCII",false} 仅占位——分量列表经 `asciiColumnNames` 下发（不走 expandedComponents）；`subcaseDisplayText(路径版)` 对 ascii 返回空串（UI 隐藏工况级，标签模板 {y.MXSubcase} 解析为空）；通道装配收口 `assembleChannelData`/`hasChannelData` 增 ascii 分支：component=列名首个匹配取列，yData=列值、xData=行号 1..N（默认横坐标，UI 侧 X 通道另以列名装配），skippedNoSecondary 恒 false，未登记/列名不存在按既有 err 风格失败；异步队列/闸门/进度/会话代次与 pch 共用（worker 双槽按 job.kind 分派，完成提交分槽保持 pch 路径逐位不动）；瞬态/频响为**块级判定**（数据形态优先：记录有无 secondary，label "IDENTIFIED BY TIME" 兜底——不再依赖文件级标签匹配，OptiStruct 等无该标签的瞬态 deck 不再被误判为频响），标志随索引叶子 NumericFmtInfo 下发，X 轴类型（Time/Frequency）与分量展开（6/12）共用；数据装配防线：分量取 secondary 而块内全无时告警跳过（不填 0 出全零假曲线）；登记键经 **normalizePath** 归一化（canonicalFilePath，防同文件因路径拼写不同登记两份/曲线 sourceFile 回查失配，filePaths 返回真实登记顺序——QHash 键序未指定另存列表）；通道装配统一 **assembleChannelData**（块匹配+防线+X/Y 装配的唯一实现，"从文件创建曲线"/"定义曲线"/"更改曲线"三方共用，防线告警经 skippedNoSecondary 标志由调用方上报） |
| 模块根 | mCurveExpressionEvaluator.h/.cpp | 表达式求值器（静态工具类）：自研引擎 mCurveMathExpression + 函数目录 mCurveMathFunctions 接入（y=f(x)+自变量x+引用按x线性插值+函数表；evaluateGrid 二元网格求值 X×Y 笛卡尔积 + evaluatePointwiseXY 逐点配对二元求值（T3）供三维定义曲线 Z 侧：网格类网格求值/曲线逐点配对），"定义曲线-数学运算"来源与预览/校验共用（见 5.5）；R40 起两 API 结果逐值清洗 ±inf→NaN（数学域 NaN 保留）+ `int* nonFiniteCount` 默认 nullptr 出参统计带出（既有调用方零破坏，2D 仅清洗断口渲染、3D 弹窗统计提示消费，见 5.13.2 修订四） |
| 模块根 | mDefineCurveDlg.h/.cpp | 定义曲线功能弹窗（单例，非模态白卡片，见 5.5）：曲线列表+向量/来源三页（文件/数学运算/数值）形态，对齐参考软件截图1~3 |
| 模块根 | mDefine3DCurveDlg.h/.cpp | 三维"定义曲线"功能弹窗（单例非模态白卡片，见 5.13.2）：左栏对象行[对象复选框+名称+类型视图下拉]+类型过滤列表+剪切/复制/粘贴/新增，矢量列 X/Y/Z/U/V，右栏 文件/数学运算/数值 三来源页；打开即载入目标三维图内全部曲线为绑定条目（20261116 修订八，对齐 2D 编辑模式；无三维图=初始空态），新增/粘贴条目类型跟随当前视图类型、默认名按条目类型生成（"曲线/曲面/瀑布图/柱状图 N"各类型独立计数不回收，20261113 修订；类型下拉=过滤视图，不回写条目类型），应用装配全部勾选条目（跨类型）写三维图（20261116 修订七起绑定原位更新+新建并存）（网格类瀑布/曲面/柱状 Z 侧数学来源=X×Y 全网格装配 rows/cols 实值入档、曲线 Z 侧数学来源=逐点配对条带 rows/cols 恒 0、y 键按活动向量侧动态启停（四类型 Z 侧均开放），见 5.13.2 修订三；R40 数学非有限值统计：部分非法建图+「N/M 个网格点无法计算，已按断口显示」模态提示、全部非法报错不建图、零非法静默——网格与条带 Z 侧数学同口径，见 5.13.2 修订四；数值页来源切换转换链/非数值侧列回显/按侧脏确认见 5.13.2 修订六；数值网格态（网格类 Z 数学→数值整网格摊平、应用按落定维度直读装配、网格态三列可编辑）与应用原位更新（editCurveId 绑定：存活原位更新/粘贴解绑/删曲线重建重绑）见 5.13.2 修订七；打开载入既有曲线（逐侧溯源还原+缺席侧数值倒表兜底+网格维度还原）与应用溯源落模（set3DCurveProvenance+模型 z3DExpression/x/y/z3DComponent 四字段+会话快照 V13）见 5.13.2 修订八） |
| 模块根 | mCurveStatisticsDlg.h/.cpp | 曲线统计弹窗（单例无边框白卡片）：超限频段统计（实测与同图目标曲线配对、方向自来源结果类型判定、线性插值、频段合并），动刚度行式/VTF·NTF 矩阵两种导出布局经 export_stats.py 生成 xlsx/pptx，见 5.12 |

### 数据流（一次属性修改的完整链路）

```
属性页控件变更 → mCurvePropPageBase::commit(prop, value)
  → mCurveDocController::setNodeProperty → push CurveSetPropertyCommand
  → (QUndoStack 调 redo) CurveDocument::setNodeProperty
  → emit dataChanged(树刷新) + nodePropertyChanged
  → 控制器转发 sig_nodePropertyChanged
  → mCurveChartView 重渲染；其他属性页实例 loadFromModel 回填
```

撤销（Ctrl+Z，树/图表焦点下）：`controller->undo()` → 命令 undo() → 文档回写 → 同一信号链路刷新所有视图。

## 2. 三类界面的实现模式

| 界面类别（设计图） | 实现方式 | 入口 |
|---|---|---|
| ① 参数栏属性页（02/03.x/04~08） | `mCurvePropPageManager::takeParamBarPage(nodeId)` 经 `mInfoWindow::editCurveNode` 挂载 | 树节点选中 / 图上点选曲线 -> sig_currentNodeChanged |
| ③ 弹窗-属性编辑器（03.1 双击曲线） | `mPropertyEditorPopup` 托管**弹窗专属页面族**（registerEditorPage 注册；未注册类型回退参数栏页），经模型信号与参数栏实例同步 | 图上双击曲线/图例项、双击图例空白、双击图表空白、双击轴标题、右键"编辑" -> sig_requestEditorPopup（工厂中继） |
| ② 功能弹窗（10~13 定义曲线等） | 非模态单例白卡片 + 信号解耦（mApplyStyleDlg 模式）；**定义曲线已重构为参考软件形态**（mDefineCurveDlg，见 5.5：曲线列表+向量/来源三页，文件/数学运算/数值三来源，表达式经 mCurveExpressionEvaluator 求值） | Ribbon"曲线"页"定义曲线"按钮（当前激活视口为曲线视窗时启用，经工厂 sig_currentChartChanged 联动） |

## 1.1 曲线选中与图例交互（设计图03.1，已实现）

交互统一收口在 mCurveChartView（画布事件过滤器 + 引擎信号）与选择中枢：

| 手势 | 行为 | 链路 |
|---|---|---|
| 单击曲线（画布） | 曲线变主题蓝+加粗1px；树同步选中该曲线节点；参数栏切曲线页 | eventFilter -> hitTestCurve -> setCurrentNodeId -> sig_currentNodeChanged -> 树回选/参数栏/高亮 |
| hover 曲线 | 仅变主题蓝，不加粗 | eventFilter(MouseMove) -> applyHighlightVisuals |
| hover 基准线 | 线变主题蓝+加粗1px，挪开/离画布恢复（视图状态不进模型；rebuild/基准线增量刷新后重放） | eventFilter(MouseMove) -> hitBaseline -> applyBaselineHoverVisuals |
| 双击曲线（画布） | 视窗左上角弹浮动属性编辑器 | eventFilter -> sig_requestEditorPopup -> 工厂中继 -> 主窗口 -> showEditorPopupAt |
| 右键曲线（画布） | 菜单：剪切/复制/粘贴（置灰占位）/编辑/删除；右键同时选中该曲线 | customContextMenuRequestedAt -> onCanvasContextMenu |
| 图例项单击 | 选中对应曲线（变蓝，同单击曲线） | 引擎 legendItemClicked（setLegendClickTogglesEnable(false) 已关闭"单击隐藏曲线"） |
| 图例项双击 | 弹浮动属性编辑器 | 引擎 legendItemDoubleClicked |
| 图例项右键 | 与曲线右键同一份菜单 | customContextMenuRequestedAt + hitTestItem |
| 双击图例空白区域（设计图08） | 激活图例属性编辑器：图例主题蓝外框 + 视窗左上角弹窗（选择中枢选中图例节点闭环） | 画布过滤器 -> hitTest 且非 hitTestItem -> setCurrentNodeId + sig_requestEditorPopup |
| 右键图例空白区域（设计图08） | 菜单仅"编辑"一项，右键同时选中图例节点（主题蓝外框） | customContextMenuRequestedAt -> hitTest 且非 hitTestItem -> 图例菜单"编辑" |
| 双击纵轴标题（左轴控件，设计图05） | 激活纵坐标属性编辑器：标题变主题蓝+加粗，视窗左上角弹 280×400 编辑器（拖右下角改尺寸） | 左轴 QwtScaleWidget 过滤器（轴标题画在画布外，画布过滤器覆盖不到）-> 标题带命中（带宽=边距+标题字高+间距近似）-> setCurrentNodeId + sig_requestEditorPopup；树上点选 Y 轴节点同样点亮标题 |
| 双击横轴标题（下轴控件，设计图06） | 激活横坐标属性编辑器：交互同纵轴（标题主题蓝+加粗、视窗左上角 280×400 弹窗，拖右下角改尺寸） | 下轴 QwtScaleWidget 过滤器 -> 最下侧标题带命中（带高=边距+标题字高+间距，自底边起算）-> setCurrentNodeId + sig_requestEditorPopup；树上点选 X 轴节点同样点亮标题 |
| 双击页眉条带（绘图区上方文字带） | 激活页眉属性编辑器：条带文字主题蓝+全行加粗，视窗左上角弹窗（弹窗页未注册 Header 类型，按回退策略落参数栏页眉页）；条带隐藏（不可见/无文本）时 Qt 不派发鼠标事件，双击天然无响应 | 条带控件过滤器（引擎 headerWidget() 访问器挂接，QPointer 防悬挂）-> bandNodeId -> setCurrentNodeId + sig_requestEditorPopup；树上点选 Header 节点同样点亮条带；选中态（_selectedBandId）经 applyHeader 按选中态着色（主题蓝+加粗，未选中=模型色 kHeaderColor/无效兜底黑），rebuild/选中切换自动重放 |
| 双击页脚条带（绘图区下方文字带） | 激活页脚属性编辑器：交互与页眉完全对称（Footer 节点/参数栏页脚页/模型色 kFooterColor） | 条带控件过滤器（footerWidget()）-> applyFooter 同页眉链路 |
| 轴区右键（四侧轴控件/副轴条带，多坐标轴） | 菜单：复制/粘贴(置灰占位)/**添加坐标轴**/编辑；hover 轴区浅蓝高亮；上/右轴标题带双击同主轴弹轴编辑器 | 轴控件+条带事件 -> onAxisZoneContextMenu -> addAxisNode(同侧叠加 X2/X3../Y2/Y3...) / sig_requestEditorPopup（见 5.6） |
| 双击图表空白区域（参考软件） | 激活图表属性编辑器：选择中枢选中图表节点（树回选/参数栏换图表页）+ 视窗左上角弹窗 | 画布过滤器双击命中链（注释->图例空白->曲线）全未命中 -> setCurrentNodeId(_chartId) + sig_requestEditorPopup |
| 右键图表空白区域（参考软件） | 菜单：粘贴(置灰占位)/编辑/**应用样式**；右键同时选中图表节点；"编辑"弹图表属性编辑器；"应用样式"弹应用样式对话框（见 5.4，设计图09） | customContextMenuRequestedAt -> onCanvasContextMenu 空白兜底分支（注释/图例/曲线全未命中） |
| 点空白（画布） | 清除选中（曲线恢复原色、树清选） | setCurrentNodeId(-1) |

- 选中/hover 高亮直改 `QwtPlotCurve` 画笔（引擎 CurveSettings 不动，图例图标本色不受影响），全量 rebuild 后重放；选中是**视图状态**，不进模型/命令栈。
- 曲线命中检测为引擎公开 API `mPlotView::hitTestCurve`（点到线段距离+容差，支持右 Y 轴曲线）。
- 树->视口反向联动：树上选中曲线节点时，所属视口同步高亮该曲线（mCurveChartView 监听 sig_currentNodeChanged）。

## 1.2 曲线树节点命名规则（对齐 HyperGraph）

树节点名分两层：**本名**（`CurveNode::name`，用户可改、进命令栈）与**显示名**（显示层现算，不存储）。

| 节点 | 本名（默认/固定） | 显示名 | 编号策略 |
|---|---|---|---|
| 工程(根) | `工程`（固定） | 同本名 | — |
| 页 | 手动新建 `Page 1`；pch 导入=文件基名 | `p<页序> : 本名` | 计数会话全局递增不回收 |
| 视口 | `XY Plot`（固定默认，可重命名） | 树=`w<视口序> : 本名`；MDI 标题=`p<页序>w<视口序> : 本名` | 同名靠前缀区分，本名允许重复 |
| 曲线 | 数据名（pch feature/定义曲线来源名）；缺省 `Curve 1` | `c<视口内曲线序> : 本名`（第一条 c1） | 计数不回收；**本名允许重复**（R23 对齐 HG；引擎渲染键=视图层唯一化键，见下方"曲线名"要点） |
| 基准线 | `Datum 1` | 同本名 | 计数不回收 |
| 注释 | `Note 1` | 同本名 | 计数不回收 |
| 主轴 | `X1`/`Y1`（固定名） | 同本名 | 位置编号 |
| 副轴 | `X2/X3...`、`Y2/Y3...` | 同本名 | 同侧现存最大号+1（防删轴后撞号） |
| Legend/Header/Footer/x/y | 固定名 | 同本名 | 单例不编号 |

要点：
- **默认名单一来源**：`CurveDefine.h` 的 `curveNodeDefaultName(type, num)`——HyperGraph 风格"类型词+空格+数字"（Page 1/Curve 1/Datum 1/Note 1），无前导零；视口默认 `XY Plot` 固定不占编号。
- **位置前缀显示层现算**：`CurveDocument::nodeDisplayText/nodeDisplayName/nodeFullDisplayName`；`data()` 的 DisplayRole 与 EditRole 已拆分（树内重命名编辑器拿纯本名，前缀不会被写回存储）。同层页/视口/曲线增删后 `refreshNumberedSiblings` 对兄弟行补发 dataChanged，p/w/c 序连续重排。
- **MDI 标题**：视口工厂用 `nodeFullDisplayName`（`p1w1 : XY Plot`——w 序是页内的，多页视口同屏须带 p 序才唯一）；图表增删（含撤销/重做）与重命名（kName）后 `refreshChartViewTitles` 全量重算。宿主视图树登记名仅创建时透传，重命名不回写（已知残留）。
- **向量引用同源**：`p1w1c1.[x|y]` 的编号与树显示序同一套位置计数（mDefineCurveDlg 建树/解析两侧实时遍历文档树序）；树上看到的序可直接写引用。
- **搜索按本名**：树过滤代理 `rowNameMatches` 经 NodeIdRole 取本名匹配，"p1" 前缀不参与搜索。
- **曲线名（R23 起允许重复，对齐 HyperGraph）**：模型名（节点名，重命名经 kName 分支同步 `series.name`）允许同名创建与重命名——addCurve/addComplexCurve/addBarCurve 与 kName 重命名不再查重拒绝（空名守卫与 trimmed 语义保留）；引擎渲染键=视图层唯一化键（无碰撞=模型名原样，碰撞按 `名#2`/`名#3` 顺序取首个不冲突键），命中测试/图例/高亮信号按键回传经视图键↔节点映射反查节点。粘贴的" (N)"自动后缀与模板应用的同名后缀为兼容保留（剪切后原位粘贴=原名回归，既有行为零变化）。页/视口名允许重复——唯一性由 p/w 位置前缀承担。
- 无持久化：节点名不落盘，命名规律调整无迁移负担（`.zcode/filltest` 本地回归用快照拷贝源码，改命名后需手动同步快照；axisfilltest 断言 X2/X3/Y2 与 max+1 算法结果一致仍通过）。

## 3. 主窗口侧改动清单（全部有先例）

- `mInfoWindow.h/.cpp`：+公有方法 `editCurveNode(int nodeId)`（仿 createCurve）
- `mMechMainWindow.cpp`：
  - dock 注册 `mCurveBrowserWindow`（key `CurveTree`，原 NVHTree 更名）
  - NVH"打开"流程的 activateDock 同步改为 `CurveTree`
  - +connect：视口工厂 sig_requestNewViewport / 选中节点→参数栏 / 图上双击与右键"编辑"→浮动编辑器
- `mMenuWidget.cpp`：原"NVH曲线"选项卡更名为**"曲线"**，保留已开发的"打开"（新建曲线入口）；新增"编辑"组置灰占位（定义曲线/基准线/注释/编辑曲线）
- `mDockLayoutManager.cpp`：dock key `NVHTree` → `CurveTree`（4 处）

## 4. pch 曲线流程（已并入框架）

"曲线模块"页签 → **打开** → 导入 .pch → 选 Subcase/结果/对象/分量 → **创建**：

1. `mOpenCurveDlg::createChartFromData()` 以 **beginMacro/endMacro** 一组命令写入模型：
   图表节点 + 逐 feature 的曲线节点（数据 `setSeriesData`、来源信息 `setSeriesSource`：文件/工况/结果类型/对象/分量，供"更改曲线"溯源）
2. 曲线名 = feature（如 "Point id 8003-MAG"），颜色沿用 NVHCurveWgt 的 6 色轮换（或绘图选项的统一线色）
3. 绘图选项映射：页眉开关→图表 kHeaderVisible，xTitle/yTitle→XAxis/YAxis 标题
4. **CurveTree 刷新与视口创建全自动**（视口工厂监听节点信号；渲染走 mCurveChartView/PlotView，NVHCurveWgt 自绘不再使用）
5. 四种布局语义不变：0=一图多线、1=按对象分图、2=按分量分图、3=单线单图
6. Ctrl+Z 一次撤销整组（树节点与视口一起消失），Ctrl+Y 重做恢复

**csv/txt 列数据（R22，同一弹窗与创建链路）**：文件过滤器扩 csv/txt，解析登记见 mCurveAsciiReader/mCurveDataSourceStore（模块表）；ascii 数据集 UI 三级化——隐藏"工况"列，结果类型=[Unknown]/对象=[Block 1]/分量=全部列名（具名列原样+空位回退 "Column N"，对齐 HyperGraph"Create Curves by File"红框口径，参考截图 design/csv-txt-curve/）；X 默认第 1 列（X 页可改选其他列，X/Y 同文件）；创建曲线名=列名，来源五元组=文件/工况空/Unknown/"Block 1"/列名，**创建落模补写 X 侧来源四元组（工况空/Unknown/Block 1/实际所选 X 列——R22-B01，x 数据页由此回显可选；装配失败退行号时不写保持未记录，pch 保持横坐标形态未记录语义）**；复数图目标按普通单曲线降级落上面板，柱状图目标照常（X=所选列，装配失败退行号）；pch 流程逐位不变（ascii/pch 切换文件时工况列显隐跟随恢复）。

框架自身的退化入口仍可用：曲线树空白处右键 → 新建图表（空图表，等待"定义曲线"接入数据源）。

## 5. 属性编辑器弹窗与参数栏内容分离（两族页面）

设计上参数栏（设计图02）与属性编辑器弹窗（03.1~08）是**两套内容**，不是同一页面的两种摆法。注册表按宿主分两族，机制层（mCurvePropPageBase 同步闭环、命令栈、折叠控件）两族共用：

| 页面族 | 注册 | 取用 | 实例策略 |
|---|---|---|---|
| 参数栏页面族（mCurvePropPages） | registerPage | takeParamBarPage | 按类型缓存单实例，同类型换节点只重绑 id |
| 弹窗专属页面族（mCurveEditorPages） | registerEditorPage | showEditorPopup | 弹窗懒创建复用；同类型只重绑 id，类型变化重建页面 |

- **回退策略**：未注册弹窗页的类型（当前除 Chart/Curve/三轴/Legend/Annotation 外全部）自动回退参数栏页面，行为与分离前一致；各类型专属弹窗页（对齐设计图04~08）逐个补齐后移除回退。
- 图表弹窗页 `mChartEditorPropPage`（对齐参考软件图表属性编辑器，双击图表空白/右键空白"编辑"激活）：基础[表头可见性·表头·图表类型(5类:XY曲线图/柱状图/复数图/极坐标图/蛛网图，XY/柱状/复数/极坐标已接渲染（极坐标见 §5.14），蛛网置灰占位)] + 高级[背景颜色·网格线颜色]。全部属性直接绑定 Chart 节点提交（页眉数据本就挂图表节点）；表头两行渲染已接线即时生效，背景/网格色随 rebuild 下发（背景色->PlotSettings.backgroundColor，网格色->各轴 AxisSettings.majorGridColor）。
- **图表类型渲染（已接线，XY/柱状/复数）**：`chartType` 0=XY曲线图（默认）/1=柱状图/2=复数图，切换走 kChartType 命令→全量 rebuild（clearAll 后逐条重画，语义为**重绘而非新增**，撤销/重做/多视口同步免费获得）。柱状映射引擎既有柱状层 CurveSettings::showBars（Qwt Sticks 垂直柱）：柱色随曲线色、隐藏折线与区域填充、柱宽按画布宽度/点数自适应（1..20px，点多收敛为细柱直方图形态）；fitChartAxes 柱状下 Y 自动范围把基线 0 包进（引擎 barBaseValue=0），否则全正/全负数据柱子悬空；选中/hover 高亮重放到柱状辅曲线（引擎 mPlotView 内联访问器 getBarCurve）。**复数图**（引擎 PlotView 2.1 `setComplexMode` 双面板共享 X 轴 + `updateComplexCurveData`，幅相/实虚两种展开）：rebuild 开头按 chartType 切换引擎面板形态；可见曲线按**命名约定配对**——pch 分量曲线名形如"对象-MAG|X-Trans"（分量词表见 mCurveDataSourceStore::expandedComponents：MAG/PHA/REAL/IMAG），去掉分量令牌即复数对键（如"8003|X-Trans"），MAG+PHA 成幅相对、REAL+IMAG 成实虚对，键含 '|' 不会与单曲线名撞键；展开方式取图内种类（有幅相对则幅相）；每对经 updateComplexCurveData 下发（a=MAG/REAL 上面板，b=PHA/IMAG 下面板），样式取上面板侧曲线的模型配置两面板同色；缺对侧/同键异种/同侧多条/两侧 X 不一致且非严格递增（重采样对齐失败）一律**降级为单曲线**落上面板，不静默丢数据；fitChartAxes 复数下下面板侧不参与 Y 适配（引擎下面板自动范围），X 照常；图例/画布命中给出复数对键，engineNodeByKey（R23 起引擎键↔节点映射，原 curveNodeIdByName）反查回 MAG/REAL 侧节点（选中/双击弹编辑器联动），applyHighlightVisuals 按键重放上下面板高亮（getLowerCurve）；非复数分量曲线（如瞬态 REAL|X-Trans 无 IMAG 对侧）照常单曲线渲染。极坐标图已由 §5.14 自绘图元族接渲染（FrameItem Z5+SeriesItem Z20 旁路自管）；蛛网图（需类别/统计维度语义+自绘多边形图元）引擎仍无绘制能力——置灰占位（模型键与 UI 均已就绪）。~~复数图 v1 已知边界（树按 MAG/PHA 两节点、下面板不响应轴属性页）~~ **已由 5.9 复数曲线单节点合并解决**（配对按模型 isComplex 分派，Phase 轴属性已接下面板）；图例/吸附 marker/十字光标/参考线/注释吸附仍仅作用于上面板。
- 曲线弹窗页 `mCurveEditorPropPage` 按设计图03.1：标准[可见性] / 曲线标签[显示标签·标签内容(曲线名称|自定义)·前缀·后缀] / 线条属性[线型·颜色·粗细·区域填充] / 标识点属性[标识样式]。
- 图例弹窗页 `mLegendEditorPropPage`（设计图08，双击图例空白/右键图例"编辑"激活）：标准[可见性·自动定位(勾选隐藏图例位置行，tooltip 说明避让策略)·图例位置(9 选项：图内四角/图内-用户自定义/图外四向)·X坐标/Y坐标(选"图内-用户自定义"才显示，画布百分比)] + 显示[边框样式(图形下拉5式:无/实线/虚线/划线/点划线)·边框颜色·字体设置("默认"占位)·引线位置(左侧/下方)·背景填充色·曲线文本色(跟随曲线颜色|自定义)·文本颜色(选"自定义"才显示)]。条件行用"整行容器+setVisible"实现完全收起。
- 坐标轴弹窗页 `mAxisEditorPropPage`（设计图05~07，三轴共用）：内容对齐参考软件的坐标轴属性编辑器，与参数栏轴页（mAxisPropPage）是**两套内容**——标准[可见性·名称可见性·文本·类型(只读回显:垂直/水平)] / 显示[标签字体设置(占位)·颜色] / 缩放设置[缩放方式·dB20参考值(占位,仅dB模式启用)·计权(已实现:kWeightMode 0无/1A/2B/3C/4U,全刻度模式生效)·最小值·最大值] / 坐标轴刻度设置[格式(kLabelFormat 0自动/1科学计数法/2定点格式)·刻度生成方式(恒显,选项集随缩放方式重建:linear 0均匀分步法→刻度总数/1固定步长法→固定步长;log/dB 0每十倍分步法(遗留默认)→每十倍刻度数/每十倍网格线数,1均匀/2固定步长同 linear 语义在刻度变换空间生效;跨 linear↔log/dB 切换重置为默认 0,同集合内 dB20↔dB10 保留)·刻度总数(kTickCount,均匀分步法)·每主刻度网格数(kMinorCount)·精度(kPrecision)·刻度字体设置(kLabelFont)]。**最小/最大值回显（弹窗与参数栏轴页同策略）**：自动范围下模型 min/max 是休眠的手动值（渲染现算不回写），回显**当前实际显示范围**——`mCurveChartView::queryAxisDisplayRange`（静态，经工厂 `chartView(chartId)` 反查所属视口，读引擎轴当前刻度区间并按轴刻度模式 inverseTransformValue 反变换；副轴与自绘条带镜像同源，含 fit/用户缩放后的现值）；回显填 spin 须**阻塞信号**（回显值与休眠值不同，setValue→commitRange 会"提交即退出自动"，打开页面不得改写模型）；提交时照旧写入并退出自动范围。CurvePages 经此单向依赖 CurveView（View 不依赖 Pages，分层不变）。新增模型属性 `kTitleVisible / kTitleColor`（CurveAxisData.titleVisible/titleColor；渲染：名称不可见清空标题、颜色映射 AxisSettings.titleColor），最小值/最大值提交时自动退出自动范围（弹窗无该行，对齐参考软件）。入口：双击纵/横轴标题——轴标题画在 QwtScaleWidget（画布外），画布过滤器覆盖不到，对左轴/下轴控件单独挂过滤器、按"最外侧标题带"近似命中（左轴=最左竖条带、下轴=最下横条带；空标题不占条带不命中）；激活态（标题主题蓝+加粗，`_selectedAxisId`）是视图状态，不进模型/命令栈，rebuild 后重放，点空白/选其他节点时恢复。
- 新增模型属性（CurveSeriesData + CurveProp 键）：`kLabelVisible / kLabelContent / kLabelText / kLabelPrefix / kLabelSuffix / kAreaFill`，默认全关/空，旧曲线零视觉影响。区域填充渲染映射 `cs.showFill / cs.fillColor`（填充色随线色；透明度0.3、基线 Y=0 为 PlotView 默认）。
- 弹窗行为：默认 **280×400**（设计图标注）；标题栏固定"属性编辑器"（白底 + M徽标 `:/mMainWidgets/Icon/logo.png`），节点身份（类型 - 名称）显示在内容区副标题行；图上双击/右键"编辑"经 `showEditorPopupAt(nodeId, globalPos)` 落位到**请求视窗左上角**；弹窗页绑定的节点被删除时（sig_nodeRemoved）自动关闭弹窗并解绑页面。**退出联动**：弹窗创建时以当前激活窗口（主窗口）为父——不作为独立"主窗口"参与 quitOnLastWindowClosed，关闭主窗口程序即退出、弹窗随父销毁；标题栏"✕"为 close()（非 hide）——弹窗作为最后可见窗口时正确触发程序退出，避免"先关软件再关弹窗后进程滞留"。

### 关键约束：任何宿主接入属性页都必须先 setNodeId

`mCurvePropPageBase::commit()` 对未绑定节点（`_nodeId < 0`）**静默丢弃**，`loadFromModel()` 回填同样依赖绑定。历史缺陷：`showEditorPopup` 创建页面后漏调 `setNodeId(nodeId)`，症状是弹窗控件显示默认值、**弹窗内所有编辑都不生效**（如"勾选区域填充无效"，其余属性因默认值恰好接近而难察觉），已修复。新增宿主形态时切记：**挂载页面之前先绑定节点**。

渲染链路问题的离屏回归工具在 `.zcode/filltest/`（未入库）：直接编译本模块真实源码，无界面渲染成 QImage 做像素断言，可分层定位（PlotView 层 / 控制器链路 / 弹窗页面链路）。

## 5.1 曲线树完善（设计图02 + 03.2，本轮已实现：树本体 + 12类属性面板）

模型层（`CurveDefine.h` / `CurveDocument`）与树/面板层对齐设计图02/03.2；**渲染新视觉分期落地**（见下方接线清单），仅接通属性页"显示"行所需的既有分支。

**曲线树本体**：
- **节点状态图标（绿色√）**：设计图02的绿色√是**状态图标**（=节点状态正常），非复选框、无交互——DecorationRole 程序化合成（绿√ + 类型图标），异常态（缺数据/失效等）后续扩展时替换字形。显示/隐藏的编辑入口在**属性面板"显示"行**与**右键菜单**。映射：Chart→`kChartVisible`（联动 MDI 视口窗口显隐，工厂监听）、Header/Footer→`kHeaderVisible/kFooterVisible`、Annotation 补 `visible` 字段（不可见注释不建渲染项）。
- **节点类型图标**：程序化绘制 16px（模块自含无新增资源）；固定子节点（轴/图例/页眉页脚/曲线 x y）不可单独删除由 removeNode 守卫（`isFixedChildNode`）约束，图标上**不叠加锁角标**（对齐参考软件树的纯 √+类型图标形态，锁角标已移除）。
- **树内重命名**：双击叶子/F2（可命名类型：页/视口/曲线/基准线/注释；有子节点双击=展开/收起不进编辑）。提交走 kName 命令；**曲线重命名联动 series.name**（原待办已闭环；R23 起重名允许，引擎键由视图层唯一化映射）。
- **子节点顺序对齐设计图**：曲线 → 图例 → 基准 → 注释 → X1 → Y1 →（Y2 紧随轴区）→ 页眉/页脚。`createNode` 按 `childInsertionRow` 分区插入；撤销删除恢复（insertNodeTree）也按分区位置插回。
- **右键菜单补全**：轴/图例/页眉/页脚节点=显示隐藏切换；图表/X轴/Y轴菜单=**"增加第二纵坐标轴"**（设计图07 入口，`addYAxis2`，每图表至多一枚、已存在置灰）；曲线菜单="编辑数据属性"（定位展开 x 子节点）。
- 筛选下拉"曲线"项含曲线下 x/y 数据子节点。

**曲线 x/y 数据子节点（设计图03.2）**：`CurveXData/CurveYData` 为曲线固定子节点（壳节点，数据存父曲线 `series.xRef/yRef`，读写经文档重定向——仿页眉页脚挂图表的先例）。属性页 `mCurveDataPropPage`（X/Y 侧共用）：
- 数据源组只读回显（文件/子工况/结果类型/对象/分量）+ "更改…"弹通道选择器换源（设计图13 更改曲线，见 §6）；
- 数据组：缩放系数/偏移量/相位角/裁剪（最大值/最小值）/单位 即时入库；用户定义式置灰占位（挂设计图12 表达式）；y 侧"坐标轴"（Y1/Y2，无 YAxis2 节点时 Y2 项置灰）。

**12 类属性面板补全**（模型键+面板行；渲染见接线清单）：
| 面板 | 本轮新增 |
|---|---|
| 01 页面 | 宽度/字体 + 布局[发布布局/发布内容位置占位]（`CurvePageData`） |
| 02 图表 | 图表类型（5类下拉：XY曲线图/柱状图/复数图/极坐标图，前四者已接渲染；蛛网图置灰占位）/背景颜色/网格线颜色（渲染已接线）（`CurveChartData`）；后续 §5.2 对齐重构：显示/名称行移除（kChartVisible 入口移至树右键），新增表头可见/表头行 |
| 03 曲线 | 弹窗页标识点属性补 标识大小/标识颜色；轴页两字体 commit 空回调接通、页眉页脚字体回填修复 |
| 06/07 页眉/页脚 | 补 颜色行（kHeaderColor/kFooterColor） |
| 08 图例 | 自动定位(避让算法已接线)/视图位置(9位置:图内四角+用户自定义+图外四向)/X·Y坐标(自定义)/边框样式(5式图形下拉)/边框颜色/字体/曲线文本色(跟随曲线颜色|自定义)/文本颜色/背景填充色/引线位置(左侧|下方)（`CurveLegendData` 扩展，渲染已接线，图外四向除外） |
| 09 基准 | 重构对齐参考软件：标准[可见性/名称/标签/图层(kLayer 新增,占位)/位置/关联轴(X1/Y1+既有副轴，切换即改方向 kOrientation，轴名入 kLink)/类型(只读回显 垂直/水平)] + 显示[字体设置(kLabelFont,已渲染)/标签位置(9宫格,默认居中骑线,kLabelPos 替代原 kLabelAlign)/线型/线条颜色/线宽(1~4px图标档)]；**出厂默认改：线色蓝(0,0,255)、实线(lineStyle=1)、线宽1.0**（kLabel/kLink 等旧键语义不变，kLabelAlign→kLabelPos）；**线上标签已渲染**（创建时标签初值=基准线名称并显示在视图中，清空不显示；字体/标签位置/线色决定标签样式与摆放，见 6 基准线行） |
| 10 注释 | 参数栏页补 显示；弹窗页 对齐方式/边框颜色/自动定位(锚点/偏移)由占位转正接模型键 |
| 11/12 X/Y轴 | 补 单位/补偿/主刻度步长/次刻度数（0/-1=自动） |

**渲染接线清单（下一期，本期未实现——相关属性行修改暂无视觉效果）**：x/y 数据变换链（缩放→偏移→相位→裁剪，位于刻度预变换前）→`applySeries`；Y2 右轴显示（启用 yRight+SafeScaleDraw+yAxis 赋值）；轴刻度步长→TecPlot 引擎 setFixedStep/setMinTickCount；增量刷新保缩放（可选优化）。图表背景色/网格线颜色已接线（rebuild 头部背景色 PlotSettings.backgroundColor、applyAxis 网格色读图表节点 majorGridColor）。
**页眉/页脚渲染（本期已接线，`applyHeader`/`applyFooter`）**：PlotView 引擎已提供页眉/页脚条带 API（setHeaderText/setFooterText/setHeaderStyle/setFooterStyle/setHeaderAlignment/setFooterAlignment，条带位于绘图区上方/下方，空文本自动隐藏，导出图像时经 `exportToImageWithHeaderFooter` 一并导出）。渲染按模型 `CurveTextData`（visible/text/font/align）下发：开关打开且有文本时显示；对齐映射 0居中/1左/2右→Qt 对齐。**注意**：引擎改动后需重建 PlotView 库并安装（头副本同步 install/include），RebuildGUI 以导入库方式消费。
**图例渲染（本期已接线，`applyLegend`）**：显隐/字体(模型字体+加粗)/背景→PlotSettings；边框样式(0无/1实线/2虚线/3划线/4点划线)+边框颜色/曲线文本色(跟随曲线颜色=按曲线色逐项着色，或自定义统一色)/引线位置(线样图标在文字左侧或下方)/选中蓝框→引擎 `mPlotLegendItem` 新增 setter（setBorderStyle/setBorderColor/setTextFollowCurveColor/setTextColor/setIconBelowText/setSelected，沿用"引擎补 setter"先例）；位置：自动定位=图内四角按"右上优先"试摆、`legendRect()` 与可见曲线采样点(约200点/线抽样)求遮挡取零遮挡/最小者，手动=图内四角或"图内-用户自定义"(posX/posY 画布百分比→setFloatingPos)。**待接线**：图外-顶部/底部/左侧/右侧（图例为画布内 QwtPlotItem 受画布裁剪，需引擎外置图例方案，暂映回图内右上）。**注意**：引擎改动后需重建 PlotView 库并安装（头副本同步 install/include），RebuildGUI 以导入库方式消费。
**曲线标签渲染（本期已接线，图例文本方案，`makeCurveSettings`→`CurveSettings::legendText`）**：勾选"显示标签"的曲线，图例条目文本 = 解析后的标签文本（原计划的 QwtPlotMarker 直挂方案废弃——标签语义落在图例文本，与绘图选项"标题设置"对轴标题的效果一致）；未勾选/解析全空显示曲线名。模板解析 `resolveCurveLabelTemplate`（mCurveChartView.cpp）：来源占位符 {x|y.MXFilePath/FileName/Subcase/Datatype/Request/Component} 取值与 x/y 数据页侧来源回显约定同源（CurveDocument nodeProperty 读写重定向：X 侧 subcase 空回退锚通道工况，schema/object/component 取 xSide*（横坐标形态为空不回退）；Y 侧 ySideSchema 空回退创建时五元组）；统计占位符 {max(y)}=Y 最大值（模型原始值）、{x}=最大值所在点 X（"At X"与"Max Y"成对语义），跳过非有限值与"编辑曲线"隐藏区间点，数值 'g'6 与读数一致；占位符来源缺失替换为空（同标题空值跳过），未知占位符原样保留。组装 `composeCurveLegendText`：前缀(受"显示标签前缀")/内容/后缀(受"显示标签后缀")解析为空的部分跳过，"-"连接（同绘图选项标题 spliceTitle 拼装效果）。引擎侧 `CurveSettings` 新增 legendText 覆盖字段（空=曲线名兜底；name 仍为图例点击命中键，图例宽度度量/绘制均按显示文本）；复数图整对图例文本取上面板侧曲线标签（配对键不变）。**注意**：引擎改动后需重建 PlotView 库并安装（头副本同步 install/include），RebuildGUI 以导入库方式消费。

## 5.2 参数栏面板对齐参考软件截图（01~08，本轮已实现）

参考软件各节点属性面板截图（01页面/02图表/03曲线/04·05曲线X·Y数据/06页眉/07页脚/08图例）为分组与参数项的最终依据；复用模块既有控件（mCurveFoldSection/mCurveColorButton/mCurvePenStyleCombo/字体下拉/数字框工厂）重组各面板，仅补设计必需的 4 个模型键：

| 面板 | 本轮调整 |
|---|---|
| 01 页面 | 标准[标题(kName)·页面布局] + 显示[字体设置(kFont)] + 基础[发布页面(kPublishLayout)·发布页面标题(kPublishPageTitle 新增)]。**页面布局=mImageGridComboBox** 视口布局图标网格（与 mOpenCurveDlg"布局"旁的网格选择同款，图标路径表抽为 `curveViewportIconPaths()` 共用，PostCurveGlobal.h），索引入新键 kLayout（首消费方=报告导出侧 20261102-R07，见 §6"页面布局渲染"行；视口平铺渲染下一期）；宽度/发布内容位置行移除（键保留） |
| 02 图表 | 基础[表头可见·表头（即 kHeaderVisible/kHeaderText，数据挂图表节点，与页眉页/树右键同键）·图表类型(5类：XY曲线图/柱状图/复数图/极坐标图/蛛网图，`setupChartTypeCombo()` 填充，XY/柱状/复数/极坐标已接渲染（§5.14），蛛网置灰占位)] + 高级[背景颜色·网格线颜色（渲染已接线：背景 PlotSettings/网格 AxisSettings）]。名称/显示行移除；**kChartVisible 编辑入口改到树右键**（addToggleVisibleAction 增加 Chart→kChartVisible 分支，联动 MDI 视口显隐不变） |
| 03 曲线 | 标准[可见性] / 曲线标签[显示标签·标签内容·显示标签前缀·标签前缀·显示标签后缀·标签后缀] / 线条属性[线型·线条颜色·线宽·区域填充] / 标记点属性[标记样式]。新增 kLabelPrefixVisible/kLabelSuffixVisible（前缀/后缀编辑行随开关整行收起）；标记样式渲染按 symbolStyle>0 派生（mCurveChartView），无需 kShowSymbol 行；名称/符号大小/符号颜色行移除（kName 树内重命名，kSymbolSize/kSymbolColor 键保留） |
| 04/05 X·Y数据 | 数据[数据源(占位:外部文件)·文件+更改(文件对话框换源，设计图13；R22 起过滤器含 csv/txt，ascii 落默认通道[Unknown]/[Block 1]/第 1 列)·子工况/结果类型/对象/分量(单项下拉只读回显；csv/txt 来源 R22：子工况行占位"-"并禁用——不得显示假 Subcase 文本，结果类型=Unknown/对象=Block 1/分量=列名)] + 标准[缩放系数·偏移量·起始值(只读回显本侧数据首值)·最大值/最小值(置灰回显数据范围)·用户表达式(置灰挂12)·坐标轴(两侧均有：X侧X1/X2 恒置灰X2、Y侧Y1/Y2 无YAxis2时置灰；**复数曲线 ym/yp 壳不显示本行**——yp 侧整行不建、ym 壳按父曲线是否复数在 loadFromModel 动态隐藏)] + 单位[类型(置灰占位)·量纲(kUnit)]。相位角/数据裁剪行移除（kPhase/kClip* 键保留） |
| 06/07 页眉·页脚 | 标准[可见性·文本·类型(只读回显:页眉/页脚，置灰)] + 显示[一级字体设置(kHeaderFont/kFooterFont)·二级/三级字体设置("默认"占位，模型仅一组字体)·对齐方式(中/左/右)·颜色] |
| 08 图例 | 标准[可见性·自动定位(勾选隐藏图例位置行)·图例位置(9选项)] + 显示[边框样式·边框颜色·字体设置(kLegendFont)·引线位置·背景填充色·曲线文本色·文本颜色(选"自定义"才显示)]。X/Y坐标行移除（编辑入口在弹窗专属页 mLegendEditorPropPage，模型键 kPosX/kPosY 不变） |

- 新增模型属性：`CurvePageData.layout/publishPageTitle`、`CurveSeriesData.labelPrefixVisible/labelSuffixVisible`（CurveProp 键 kLayout/kPublishPageTitle/kLabelPrefixVisible/kLabelSuffixVisible；默认值 layout=0/publishPageTitle=true/两可见=true，旧数据零影响；页面布局自 20261102-R07 起在报告导出侧消费（`curvePageLayoutGrid` 派生表，见 §6），曲线标签渲染已接，图例文本方案见 5.1 渲染清单；其余仍待渲染接线）。
- 勾选行统一为"勾选框在左、文字在右"的整行形态（`createRowCheck` + addRow 单参重载），对齐参考截图。
- 对齐下拉文案改"中/左/右"（索引语义 0中 1左 2右不变，页眉/页脚/基准线共用）。

## 5.3 线型/线宽/符号图标对齐绘图选项 + 注释/轴参数栏对齐截图（10~12，本轮已实现）

**绘图选项同款图标下拉（mCurveEditorWidgets，属性页与绘图选项曲线页共用一组 SVG）**：
- `mCurvePenStyleCombo` 线型：4 项换为 `:/mMainWidgets/Icon/NVH/` 下 实线/虚线/点划线1/点划线2 图标（纯图标无文字，同 mDrawingOptions），索引语义不变（曲线侧=TecLinePattern 0..3，基准线侧 +1=Qt::PenStyle）——曲线/基准线属性页与弹窗曲线页同步生效；
- `mCurveLineWidthCombo` 线宽（新增）：1px/2px/3px/4px 样本图标，索引 0..3=线宽 1..4px（曲线页提交 kLineWidth=idx+1，回填就近取整钳制 1..4 档）；注释页"边框线宽"同用（kBorderWidth 同映射）；
- `mCurveSymbolStyleCombo` 符号（新增）：10 种符号图标（标识无/添加(圆)/方/菱形/三角/十字/倒三角/星形/交叉/圆点），索引=kSymbolStyle 0..9；
- `mCurveBorderStyleCombo` 图例边框样式：0=无（∅ 自绘）保留，1..4 项换用同组线型图标（索引语义不变）。

**注释参数栏页（截图10）**：标准[可见性/名称/文本(QLineEdit 直编 kText)/标量值格式化(勾选显 格式/精度)/附着于/附着曲线/附着点/位置(X·Y 并排)] + 选项[最小化(勾选显 半径/颜色)] + 显示[字体设置(只读"默认"占位)/对齐方式(左/中/右=kAlign)/边框线宽/曲线颜色(勾选=kBorderMode 0 跟随曲线颜色，取消显 选择边框颜色)/自动定位(勾选显 策略/偏移)/背景颜色·文本颜色(默认=无效色|自定义+取色行)]。类别切换/颜色无效色语义与弹窗注释编辑页一致。

**X/Y轴参数栏页（截图11/12，与弹窗 mAxisEditorPropPage 同构）**：标准[可见性/名称可见性(kTitleVisible)/文本(kTitle)/类型(只读回显 水平/垂直，置灰)] + 显示[标签字体设置(kTitleFont)/颜色(kTitleColor)] + 缩放设置[缩放方式(默认四项 linear/logarithmic/dB20/dB10；复数图 X1 仅 linear/logarithmic、幅相展开 Phase 仅 linear——词表收窄，历史存量越表值**显示与渲染均按钳制值**(X1→Logarithmic、幅相 Phase→Linear)，模型值不动；实虚展开 Phase 四项全开)/dB20参考值(仅 dB 模式启用)/计权(已实现:同弹窗页,全刻度模式生效)/最小值/最大值(改范围即退出自动，无"自动范围"行)] + 坐标轴刻度设置[格式(kLabelFormat 0自动/1科学计数法/2定点格式)/刻度生成方式(恒显,选项集随缩放方式重建:linear 0均匀分步法→刻度总数/1固定步长法→固定步长;log/dB 0每十倍分步法(遗留默认)→每十倍刻度数/每十倍网格线数,1均匀/2固定步长同 linear 语义在刻度变换空间生效;跨 linear↔log/dB 切换重置为默认 0,同集合内 dB20↔dB10 保留)/刻度总数(kTickCount,均匀分步法)/每主刻度网格数(kMinorCount)/精度(kPrecision)/刻度字体设置(kLabelFont)]。原 单位/补偿/主刻度步长/自动范围行移除（kUnit/kAxisOffset/kMajorStep/kAutoScale 键保留，自动范围仍由改范围退出语义触达）。

**曲线标签模板交互（截图03标注，参数栏曲线页与弹窗曲线编辑页同交互）**：标签内容/标签前缀/标签后缀换用 `mCurveTemplateCombo`（mCurveEditorWidgets 新控件）——点下拉箭头弹出勾选变量面板（顶部三个操作按钮：**全选/全取消/互换**，程序化图标，按钮行与列表间有分隔线；变量表 `curveLabelVariables`：X/Y 各 6 项来源信息占位符 {x|y.MXFilePath/FileName/Subcase/Datatype/Request/Component} + Max Y=Max={max(y)} + At X=At X={x}），勾选项按列表顺序以"~"拼接成模板文本（如 "{y.MXRequest}~{y.MXComponent}"）；箭头前编辑区可直接输入任意文本（可编辑下拉，回车/失焦提交并按"文本包含片段"回设勾选态）。标签文本入 kLabelText/kLabelPrefix/kLabelSuffix（kLabelContent 键停用保留）。默认模板 {y.MXComponent} / {y.MXRequest} / Max={max(y)} 来自 **CurveSeriesData 出厂初值**（新建曲线即带默认；回填原样显示模型值——不做"空值换默认"替换：控制器 setNodeProperty 对 null 旧值与空串的等值短路会吞掉清空提交，导致"全取消/互换"后勾选复原、切节点又变回默认；既有旧曲线该三字段为 null，面板显示为空，重新勾选/输入即写入）。取消"显示标签"则下方标签设置项全部**置灰**（不隐藏，对齐截图标注），前缀/后缀开关各管其下拉可用态；取消"可见性"曲线连同标签一并隐藏（曲线渲染已通；标签渲染=图例文本方案，见 5.1 渲染清单）。**置灰可见性**：参数栏宿主/属性编辑器 body 的 `QWidget{background:white}` 等祖先样式表会盖过应用级 ：disabled 规则（越近的祖先样式表优先级越高），mCurveFoldSection 层已补齐 disabled 规则，mCurveTemplateCombo/mCurveCheckBox 再以**自身样式表**兜底（自身样式表优先级最高，任何宿主规则都压不过）——灰底 #F5F7FA/灰字 #A8ABB2 与应用禁用配色一致；下拉面板中变量列表带完整 #E4E7ED 边框（包裹按钮下方内容）。

## 5.4 应用样式（设计图09，已实现）

图表背景右键 -> **应用样式** -> `mApplyStyleDlg`（模块根，功能弹窗单例，非模态）：
把**源图表**（右键来源）的样式按右栏勾选的属性类别，批量复制到左栏勾选的页/图表。

- **左栏"作用对象筛选"**：`仅可见曲线`复选框（默认勾选）+ 作用范围下拉（全部页面/当前页面/最后一次选择）+ 全选/全不选/反选图标按钮 + `工程->页->图表`三态复选树（勾选页=应用到其下全部图表；源图表默认不勾且应用时强制跳过）。"最后一次选择"=会话内记忆上次应用集（不落盘，节点 id 跨会话不稳定）。对话框存活期间删除页/图表 -> 树自动重建剔除失效行；删除源图表（或其祖先页）-> 对话框自动关闭。
- **右栏"选项"**：七组属性叶子（默认全勾，状态随实例保留）——Plot属性[背景色/网格线颜色]、图例[字体/边框图颜色/颜色(背景+文本色)/位置/可见性]、页眉·页脚[一级字体/颜色/对齐方式/可见性]（挂图表节点；**文本内容不属于样式不复制**）、坐标轴[应用到(水平主轴/主垂直轴/全部副轴)+外观属性(标签字体/刻度字体/颜色)]、标记设置[坐标轴缩放/最小值/最大值/增量(主刻度步长)/网格数量(次刻度数)/自动适配]（同样受"应用到"范围约束；最小/最大写入附带退出自动范围）、曲线[线型/颜色/线宽/标记样式/大小/颜色/区域填充]。设计图中模型无对应的子项（页眉二级/三级字体、刻度方式、刻度数量、数据格式/精度、清空数据重绘图）暂不列入，待模型扩展后补。
- **曲线按序配对**：目标第 i 条曲线 <- 源第 i 条曲线样式，源不足时目标剩余不动；"仅可见曲线"勾选时源/目标两侧都先滤掉隐藏曲线再配对。
- **执行**：底部按钮 应用(应用并保持打开)/取消(关闭)/确定(应用并关闭)。全部写入经 setNodeProperty（值相同自动跳过）包 `beginMacro("应用样式")` -> **一次 Ctrl+Z 整体撤销**；目标无对应轴节点（如无 Y2）跳过。跨页多视口经属性变化信号自动重渲染。
- **形态**：照 mPropertyEditorPopup 的无边框白卡片（M徽标标题栏+✕，拖动/右下角缩放），DlgGeom::bind(this, "ApplyStyleDlg") 记忆大小位置；父窗口沿 parentWidget 链走到顶层主窗口（QMdiSubWindow 亦带 Window 标志，直接作父会被 MDI 收编）。


## 5.5 定义曲线——新增曲线（设计图10~12 + 参考软件截图1~3，本轮重构完成）

Ribbon"曲线"页 -> "定义曲线"按钮 -> `mDefineCurveDlg`（模块根，功能弹窗单例，非模态）。
按钮**仅在当前激活视口为曲线视窗时启用**：视口工厂信号 `sig_currentChartChanged`
（MDI 视口激活变化时广播当前图表id，非曲线视口为 -1），mMenuWidget 据此启停。

**形态对齐参考软件（截图1~3），2025-09 重构**：原"数据集树勾选通道+已选表"形态
废弃，改为参考软件的"曲线列表 + 向量/来源定义"形态：

- **左栏"曲线列表"**：曲线名称复选框（勾=用名称编辑框命名，不勾=按来源自动命名）
  + 待新增曲线列表 + 剪切/复制/粘贴/新增按钮。**状态机对齐截图**：打开时（图1）
  除"新增"与底部"关闭"外全部置灰；点击"新增"产生"曲线 N"条目（编号会话内自增），
  条目入列表并选中，右栏与剪切/复制随之可用（图2）；粘贴仅在有剪贴板内容时可用
  （对话框内剪贴板，复制/剪切条目整体克隆）；每次打开弹窗复位到图1初始态
  （未应用的定义不落模型）。
- **右栏"曲线定义"（随左栏选中条目联动）**：
  - **向量**：X/Y 单选 + 回显编辑框（X=横坐标类型/曲线引用，Y=分量/表达式镜像只读）
    + fx 按钮（弹"曲线选择器"）；
  - **来源**：文件 / 数学运算 / 数值 三单选切 QStackedWidget 三页。
    **20261106（R20）起常规 XY 与复数图目标"来源"按向量侧独立**（对齐
    HyperGraph，设计稿 design/define-curve-per-vector-source.png）：一条条目
    的 x/y（复数图 x/yp/ym 三侧）各自持有来源（`DefineCurveEntry`
    xSource/ySource/ypSource/ymSource），"来源"单选只写激活侧。
    **20261109（R21）交互模型修订**（推翻 R20"切向量跟随显示该侧来源"）：
    页面只随"来源"单选切换，点向量=选中该侧并套用当前页来源——该侧有效
    来源==当前页来源时仅切焦点（文件页四级列编辑目标/数学页插入与拾取目标/
    数值页激活列），不同则按当前页来源转换该侧（applySourceChange 共用链路：
    脏确认→数学暂存→倒表预填 B02→x 快照 B03→落值落定，与点"来源"单选完全
    同链路），脏确认取消则来源+向量两单选组一并回退，点向量永不跳页；
    **不变式：非柱状下"激活侧来源==来源单选"恒成立**（切条目/粘贴/切型后
    按 sideSource(activeVector) 回显维护）。**按侧来源带哨兵语义（R20-B01）**：
    新建条目各侧 -1=未显式设置，未显式设置侧随最近一次显式选择显示（整条
    source 兼任镜像），首次选择来源即全侧落定、此后各侧只随各自的显式切换
    分叉——新建条目选数学运算后切向量不闪回文件页；编辑模式载入写入的都是
    显式值；粘贴/切型播撒不注入哨兵，显式值得以保留；数学暂存（x/yp/ym 侧
    各一份，XY 的 Y 侧与表达式框同源免暂存）与"离开数值"脏确认（xValueDirty
    等按侧）均按侧
    执行，确认框文案不变；文件路径下拉也按侧独立（每侧各自 filePath，切向量
    回显该侧文件，两侧可同选一文件；导入后选中到激活侧；X 侧结果类型并入 Y
    侧工况结果类型的规则仅同文件时生效）。**柱状图目标维持整条 `source` 语义
    不拆**（共享数值表/四级列/表达式交互零变化，按侧访问器在柱状图目标下统一
    退化映射整条字段）；
  - **文件页**：数据文件下拉（mCurveDataSourceStore 共享数据集，"导入..."可直接补
    导入 pch/csv/txt，R22 起过滤器含 csv/txt）+ 工况→结果类型→对象→分量 四级联动
    单选列表（规则与"从文件创建曲线"完全同源：仅矢量结果、分量展开、多数值格式
    跳过）；四级列编辑目标=激活向量侧通道（各侧独立保存）；选定分量后 Y=通道
    "对象-分量"，X=数据块横坐标（回显 Time[Sec]/Frequency[Hz]）或 X 侧所选通道；
    **csv/txt 数据集（R22）本页退化为三级**：隐藏工况列（pch 不变），结果类型=
    [Unknown]/对象=[Block 1]/分量=全部列名（具名列+空位回退 "Column N"）；X 侧
    默认第 1 列，pch"块横坐标"兜底链路（含 R20-B03 x 快照）对 ascii 一律回退
    第 1 列；自动命名=列名；应用落模五元组工况文本=空串（回显经
    subcaseDisplayText(路径版) 为空）；R20/R21 按侧来源状态机不加 ascii 特判，
    仅在列填充/装配/命名取值处按 fileKind 分流；
  - **数学运算页（图3 表达式构建器）**：函数/曲线两选项卡（函数库分类+搜索过滤；
    曲线页=文档 页p→视口w→曲线c→x/y 引用树）+ 名称/值参数表（选中函数的参数行
    可编辑、fx 选择器填引用，实时合成调用串）+ 直接插入（当前向量：X=替换引用，
    Y=插入表达式光标处）/用于表达式 + 表达式编辑框 + 计算求解复选框 + 值预览表
    （前 8 个采样点，出错红字显示原因）；构建器/曲线拾取/函数参数 fx/预览表
    机制不变，仅作用目标侧跟随激活向量侧的来源（R20）；
  - **数值页**：常规 XY=Pt|X|Y、复数图=Pt|X|Yp|Ym 表格（Pt 只读行号）——
    **仅激活向量列可编辑**（R21；剪切/复制/粘贴只落激活列）。非激活列只读：
    数值侧列展示其数值串（数据保留），非数值侧列回显该侧当前来源装配的实际
    数据（'f' 6，行位对齐、超出留空，切入数值时把激活侧旧来源内容装配进该侧
    列供微调——倒表在侧来源落新值之前装配切出侧内容，R20-B02 修前调用点在
    落值后致倒表从未生效；首次显式选择全侧落定为数值时其余列无预填，属按侧
    口径；该侧通道/表达式改动后再进数值页刷新）；剪切/复制/粘贴只作用于
    可编辑列（剪贴板 token 按位映射可编辑列），非法数值红字只标可编辑列；
    行操作工具条 新增行/删除选中行/清空全部行仍作用于整表行（新增行可编辑格
    置 0、只读格按行位回显；清空钮暂以文字占位，图标后续提供，资源库现无清空
    类图标）；柱状图目标为共享数值表（全列可编辑，现状不变）。
- **曲线向量引用 `pNwMcK.[x|y]`**：按文档结构解析（页/视口/曲线均按树内显示序
  1 基编号，仅数 Curve 类型子节点；新增曲线插入曲线区末尾，既有引用编号稳定）。
  fx 按钮（向量行/参数格）弹**曲线选择器**浮动面板（Qt::Popup 白卡片，双击 x/y
  叶子即插入并关闭）；x 向量引用取值=曲线 X 数据（自变量=采样索引），y 向量引用
  在求值时按其 X 数据线性插值到采样点（区间外取端点）。
- **求值内核 `mCurveExpressionEvaluator`**（模块根新文件）：自研表达式引擎
  `mCurveMathExpression`（词法/递归下降/AST，四则+幂^+一元±+括号+函数调用，
  替代原 muParser，无第三方依赖）+ 函数目录 `mCurveMathFunctions`（《函数类型.xlsx》
  目录与实现唯一样）。y=f(x)：自变量直接写 `x`；引用名含 '.' 不在引擎变量名字符集，
  求值前整体替换为双下划线形式（p1w1c1.y→p1w1c1__y）并按引用注册变量；
  pi/e 常量由引擎预置。函数表（基础数学/统计特征值已实现，插值/频谱等目录项
  红色占位）供构建器展示；聚合/向量变换（mean/rms/integral/delta...）在求值前
  预替换为常量或"虚拟向量"。
  **计算求解**复选框：勾选=按 X 采样逐点求值；不勾=表达式必须为单个直接引用
  （数据复制语义）。X 采样取 X 向量引用的值序列，未指定时取首个引用曲线的 x。
- **确定执行（底部改为 关闭/应用）**：`beginMacro("定义曲线-新增曲线")` 逐条目
  addCurve + setSeriesData + setSeriesSource（文件=五元组；数学运算/数值=来源名
  +表达式/数值串回显）+ 数学运算来源同时把表达式/X引用写入 x/y 壳节点
  kExpression（设计图12 挂点，模型字段此前预留）+ 轮换配色（自目标图表既有曲线数
  续起）+ 线宽2.0；一次 Ctrl+Z 整批撤销；应用成功后弹窗复位到图1初始态（保持
  打开可继续定义）；不完整条目警告跳过，全部失败不复位。
  **R20 混合来源装配**（常规 XY/复数图按向量侧来源）：各侧按自身来源装配取值
  序列（文件=该侧 filePath+通道经 assembleChannelData 同源装配，X 未选通道取
  主侧文件块横坐标——现状口径；主侧切离"文件"后该回退断源：切离时刻
  （onSourceChanged 落新值前）把当时可解析的横坐标快照进 x 侧数值串（仅串空
  时写、程序快照不置脏），装配端 X 侧文件分支加第三级回退——主侧文件回退不
  可用时按行位解析 x 侧数值串兜底（非法格退行号），回显/混合应用/x 切"数值"
  倒表预填同走此链，横坐标不断源（R20-B03）；数值=该侧数值列按行解析，X 列
  空/非法退行号、
  值列按 0 兜底行位不丢；数学=该侧表达式/引用求值，采样优先另一数值/文件侧
  数据，两侧均非数值/文件时现状采样口径，solve 不勾=单引用直接取值），按行位
  配对、**行数取较少者截断**（用户拍板）；全数值/全数学条目走既有
  buildValueData/buildValuePairData/buildMathData/buildMathPairData（现状口径
  回归）。模型单来源字段跟随主侧（XY=Y 侧、复数图=ym 幅值侧），非主侧摘要/
  表达式按其自身来源落模（壳节点 kExpression 仅该侧为数学时写）。完整性判定
  entryComplete/自动命名按各侧自身口径（主侧必选、X 侧可选有兜底）。
  **已知边界**：模型每个来源字段仅一份（CurveSeriesData 单来源五元组+侧四元组），
  混合来源条目应用后按主侧来源落模——编辑模式载入时各向量侧按模型来源字段
  整体还原（数值曲线三侧全数值并倒数据；数学曲线全数学+壳表达式；文件曲线
  全文件+侧通道），x 侧曾为数值/数学的混合条目重开时退化为文件横坐标/空引用
  口径，属模型字段所限的保真边界。同源条目（全文件/全数学/全数值）应用走既有
  装配口径（全文件保留点数校验与显式报错，不静默截断）；混合条目数值列非法格
  按 0 兜底保持行位，与全数值条目（buildValueData 跳过非法 token）口径不同。
- **共享数据源仓库**（mCurveDataSourceStore，单例）：两弹窗看到同一份已导入
  数据集；新文件经 sig_fileAdded 通知，定义曲线重建文件下拉并自动选中新文件。
- **形态**：无边框白卡片（M徽标标题栏+✕，可拖动缩放），`DlgGeom::bind(this,
  "DefineCurveDlg")` 记忆几何；目标图表绑定/存活期间不漂移/被删自动关闭
  （mApplyStyleDlg 先例）不变；文档结构变化（加删页/视口/曲线）自动重建引用树。

## 5.6 多坐标轴——添加坐标轴（对齐 HyperGraph，已实现）

图表四侧**轴区**（下X1/左Y1 主轴区 + 上X2/右Y2 副轴区 + 自绘条带）支持叠加任意数量副轴：

- **入口（轴区右键菜单）**：轴控件（QwtScaleWidget）与自绘条带的整个区域即热区，
  右键弹出：复制(置灰占位)/粘贴(置灰占位)/**添加坐标轴**/**编辑**。X 侧（下/上轴区）
  添加顶部横轴（默认名 **X2/X3...** 按同侧既有数递增），Y 侧（左/右轴区）添加右侧
  纵轴（默认名 **Y2/Y3...**）；副轴热区可继续右键再叠加（X3/Y3 起）。hover 轴区显
  浅蓝高亮（鼠标穿透覆盖层）。添加走 `mCurveDocController::addAxisNode(chartId,
  vertical)`：一个撤销宏 = 新建节点（XAxis2/YAxis2 类型）+ 初值复制同侧主轴（刻度
  模式/范围/字体/颜色，新轴初始与主轴刻度对齐）+ 标题=轴名，Ctrl+Z 一次撤销。
  树右键（图表/轴节点）提供"添加横坐标轴/添加纵坐标轴"同款入口。
- **编辑**：菜单"编辑"/双击轴标题带（上/右轴对称扩展了命中逻辑）-> 轴属性编辑器
  （mAxisEditorPropPage，XAxis2 已注册两族页面；类型回显 水平/垂直）。
- **渲染分工**：引擎只有 4 根原生轴——每侧**首枚副轴**（X2/Y2）映射引擎 xTop/
  yRight（启用刻度/数字组件、mCurveScaleDraw 'g'4 格式、不画网格防双线）；**第2枚
  起**（X3../Y3..）由 `mCurveAxisStripWidget`（CurveView 新控件）自绘：刻度经
  QwtLinearScaleEngine 生成、范围取模型（手动范围按刻度模式变换）或镜像同侧原生轴
  当前范围。**摆位分两路**：顶部条带经 `indexOf(qwt)` 插入 mPlotView 内部布局
  （页眉与 QwtPlot 之间；同位点反复插入=由内向外堆叠，X3 紧贴 X2、页眉保持最顶，
  几何由 Qt 布局自动排布）——条带画布横向对位在 paintEvent 内现算（条带横跨引擎
  全宽，刻度须以画布左右边缘映射）；右侧条带仍在视图右外扩边距内手动几何、纵向
  与画布对齐（右侧无页眉类条带冲突；右边距注意 setContentsMargins 参数序
  left/top/right/bottom——曾把条带总宽误传成**左**边距，引擎整体右移致"左轴旁
  多一条空白列 + 条带被推出视图外"，改后设置边距即 activate() 再取映射，防布局
  延迟激活读到旧位置；rebuild/画布尺寸变化经 `scheduleAxisStripLayout` 延迟重排
  ——Resize 可能在布局激活中被同步派发，事件回调里同步改布局会重入崩溃）。
  **显式同步入口 `syncSelfDrawnGeometry`（公开，幂等）**：上述"画布 Resize→0ms→
  updateAxisStrips"链的终点在事件循环兑现，而报告导出对懒物化视图"物化→grab"
  全程同步、不进事件循环——自绘层几何停留构造期（20261102-B02：边框覆盖层成
  "绘图区左上角小框"、整图无外框，右侧条带同停构造位）。该入口先激活本视图与
  引擎布局再走 updateAxisStrips 收口，由导出守卫（mExportReportDlg 的
  OffscreenPaintReady）截图前显式调用；已同步视图重复调用为同值写短路（对正常
  显示页零干扰），只写几何不碰缩放/激活态/焦点。
  **20261103-R10 起导出走临时视图重渲**：报告导出采集层不再物化/借用界面卡片
  视图（B02 时代"导出物化→grab"的 live-view grab 路径随之一并退役），改为
  mExportReportDlg 匿名函数 `grabChartStandard(chartId)` 构造临时
  mCurveChartView 绑定目标图表、统一按标准尺寸 1515×765 离屏重渲染（此前
  `view->grab()` 跟随界面卡片实际大小：显示过页 755×368 文字占比大、未显示过页
  经 B02 同步为整幅 1515×765 文字占比小，R08 拉伸充满放大了该不一致——用户可
  感知"显示页字大色深、未显示页字小色浅"）。临时视图**不进视口工厂登记**
  （无卡片/无 attach/无 LRU 登记，零界面干扰），与本守卫走同一条就绪链路
  （resize→WA_DontShowOnScreen+show→事件循环兑现 0ms 延迟链→
  syncSelfDrawnGeometry 显式收口→grab）——本入口的消费方从"被借用界面视图"
  变为"临时离屏视图"，语义不变；隐藏图表（kChartVisible=false）在临时视图下
  按完整内容渲染导出（不受卡片显隐影响，是否跳过待 B03 立规）。回归固化于
  workspace tools/r12_harness（三状态同字号逐像素一致/源图恒 1515×765/零界面
  干扰/隐藏图表行为/性能实测）。
  条带自绘背景=图表背景色（与引擎 `setMarginBackground` 同源，不透宿主底色），
  视图自身边距区（右侧条带列）亦随图表背景色 autofill——修复"条带露窗口灰底"。
- **曲线挂轴**：y 数据页"坐标轴" Y2/X2 项按本图表副轴存在性启停（X 侧原恒置灰已
  放开）；applySeries 按挂轴设置把曲线数据下发到引擎右/上轴（axisAssign=1），fit
  按挂轴曲线适配范围（无挂载曲线回退全部曲线）。注意：X3+/Y3+ 无引擎轴可挂，
  数据页仅提供到首枚副轴。
- **树/样式适配**：XAxis2 为固定子节点（不可删，误加经撤销），图标复用 X 轴切图，
  插入轴区尾部（堆叠序=创建序）；类型筛选"坐标轴"含副轴；应用样式"全部副轴"
  覆盖 XAxis2+YAxis2 全部节点按序配对。
- **离屏回归**：`.zcode/filltest/`（未入库）axisfilltest 直接链接主构建 PostCurved
  导入库跑像素断言（需 QT_QPA_PLATFORM=offscreen + QT_PLUGIN_PATH + QT_QPA_FONTDIR
  指向 Windows 字体 + setUseOpenGL(false) 切 CPU 画布；运行需按 MxMech.vcxproj 的
  LocalDebuggerEnvironment 拼 PATH——Qt/qwt/Basic 各 install/bin）：基线无轴
  带干净/加轴出现刻度文字/曲线挂 Y2 渲染/**连加两枚 Y 轴（Y2+Y3）右侧两列刻度数字
  且左轴旁无空白列**/逐步撤销复位/**页眉存在时顶部条带在页眉
  下方**（条带挂引擎内部布局的结构性断言 + 页眉文本行早于左侧刻度数字行的像素顺
  序断言；页眉开关用文档直写不进命令栈，避免污染撤销探针计数），ALL PASS。期间
  修复两处真实缺陷：轴控件过滤器对非鼠标事件强转 QMouseEvent（UB 崩溃）、画布边
  框覆盖层在 setUseOpenGL 重建画布后悬垂（改 QPointer + ensureCanvasFilter 重建）。

## 5.7 标记点属性改版 + 标题模板 + 缩放保留（本轮已实现）

**标记点属性（参数栏曲线页 + 属性编辑器弹窗曲线页，两页一致）**
- 行集改为 标记样式/符号颜色/符号密度/缩放尺寸：选"无"整组隐藏，选非空标记现出（`mCurveFoldSection::setRowVisible`）
- 符号密度 = 标记间隔（1=逐点，N=每 N 点画一个，引擎 skip=密度-1）
- 缩放尺寸 = 倍率系数（6 位小数，默认 1.0），实际大小按"基准大小×系数"推导：**大圆点(样式1，实心大圆)基准 12px，其余（含小点样式9）8px**
- 模型新增 `symbolDensity`/`symbolScale` + `kSymbolDensity`/`kSymbolScale`；`symbolSize` 字段保留为遗留不再驱动渲染
- "应用样式"的标记大小拷贝按各曲线自身基准折算缩放尺寸下发（来源/目标样式不同也保证像素大小一致）

**标题模板（页眉/页脚/X/Y 轴标题，对齐 HyperGraph 标题组成）**
- 建图默认：页眉/页脚 `{p1w1c1.y.MXRequest} - {p1w1c1.y.MXComponent}`；X 轴 = X 数据源选中的结果类型文本；Y 轴 = `{p1w1c1.y.MXDatatype}`
- 命名规则：`{[p页w视口c曲线.][x|y.]MX字段}`——字段 MXFilePath/MXFileName/MXSubcase/MXDatatype/MXRequest/MXComponent，统计 `{max(y)}`/`{x}`，大小写不敏感；未知占位符原样保留（HW 前缀写法不解析）
- `p1w1c1` 位置引用（编号规则同 1.2 树显示/"定义曲线"引用）：按引用曲线解析，图表增删后按当前结构动态解析；无引用占位符按本图表首条可见曲线解析
- 实现：`resolveBandTemplate`/`findCurveByPositionRef`/`resolveLabelToken`（mCurveChartView）；曲线标签仍按本曲线解析（不带位置引用）
- X 轴标题为横坐标形态（Time[Sec]/Frequency[Hz]）时落原文本（横坐标形态的来源类型位随换源约定复位，模板无值可解析）

**缩放保留（全量重建不再丢失用户视图）**
- `scheduleRebuild(preserveView)`：rebuild 首尾捕获/恢复"用户缩放过"轴（`axisAutoScale==false`）的分隔；数据形态键（刻度模式/计权/挂轴/图表类型）变化或重建前无可见曲线（首条曲线入图）时仍按数据重新适配
- 外观键（线色/线宽/线型/区域填充/符号五键）与节点增删路由 preserveView=true；符号/外观键另走**增量刷新** `scheduleSeriesStyleRefresh`（0ms 合并，`applySeries` 直发引擎不经重建）——无闪烁无缩放跳动
- 复数图与编辑曲线会话仍走全量（成对下发/压暗重放语义在重建内）
- 图例边框线宽：参数栏图例页"边框样式"行改为"边框线宽"（新 `mCurveBorderWidthCombo`：0无边框+1..4px 线宽样本图标，新键 `kLegendBorderWidth`）；原"边框样式"（线型语义，`mCurveBorderStyleCombo`）保留于图表编辑器；PlotView `mPlotLegendItem` 新增 `setBorderWidth`，线宽 0=无边框（笔 NoPen）、从"无"改回线宽时线型若为 0 恢复实线
- 图例文本清空 = 图例留空（`legendFallbackToName=false`，不再回退曲线名）

## 5.8 树内容与渲染行为按图表类型声明化（ChartTypeProfile，本轮已实现）

"每种图表类型长什么样"收口为一份类型声明（`CurveCore/ChartTypeProfile.h/.cpp`）：
新增/调整图表类型 = 注册/修改一份 profile，不再散落 switch。树控件三件套与文档模型
未动，**行为零变化**（5 份注册内容=原硬编码行为逐一对照搬运）。

- **结构与分层**：`ChartTypeProfile{ 树内容节 + 渲染行为节 }`；查表入口
  `chartTypeProfile(chartType)` 未注册/越界回退 XY(0)（与属性页回退策略同款）。
  表放 CurveCore——CurveDocument 与 mCurveChartView 都是消费方，依赖方向不变；
  头文件只前向声明 CurveDocument/CurveNode，displayOverride lambda 实现在 .cpp。
- **树内容节消费点（本轮全部接线）**：
  - `createChart`：固定骨架按 `fixedChildren` 循环创建（含轴默认可见 `visible`，
    当前全注册 true；日后"某类型默认隐藏某轴"改表即可）；
  - `childInsertionRow`：分区序查 `chartZoneRank`（zoneOrder 下标，表外=尾部区，
    与原 nodeZoneRank default=4 等价），**原 nodeZoneRank 已删**；副轴紧随轴区的
    特例保留（与图表类型无关）；
  - `nodeDisplayText`：显示名覆写先查 `displayOverride`（复数图 y 子节点 ym/yp，
    父曲线名令牌判定原样），再查 `fixedChildren[].displayName`（复数图 Y 轴
    Magnitude），原 chartType==2 特判整段迁出；X 轴"X1"/图例"Legend"两条与本名
    相同的冗余分支消失；
  - `mCurveTreeView::contextMenuEvent`：16-case 大 switch 改为查表组装——
    `menuActionsFor`（图表后代按所属图表查 `profile.menus`；空白/工程/页本地常量；
    menus 未列出的类型返回空=不出菜单，同原 default:return）+ `appendMenuAction`
    （TreeMenuAction→菜单项+命令 lambda，原 case 体原样搬入；"删除图表"文案不用
    显示名"视口"，沿旧文案）；
  - 词表顺带：`CurveDefine.h::nodeVisibleToggleProp`（显隐属性键按类型映射）、
    `nodeTypeRenameable`（flags 可编辑判定）；筛选按钮定义表化 `kCurveFilterDefs`
    （mCurveBrowserWindow：图标+tooltip+类型集合一张表，slot_filterChanged 的
    switch 删除）。
- **渲染行为节（一期仅注册未消费，零渲染变化）**：`renderKind`（XYLike/Bar/Complex，
  预留收编 rebuild 里 chartType==1/2 画法分支）、`zeroLineStyle`（Cross=现状十字 /
  BaselineOnly=仅水平基线"一"，柱状目标形态，配合引擎 barBaseValue=0）、
  `xTickSemantic`（NumericAuto=现状 / Category=仅用户设置类别刻度不自动外推）。
  **注册纪律**：字段翻转注册值（改行为）之前，必须先在 CurveView 接好消费点且
  注册值=改造前行为——"加字段"与"改行为"两个动作分离、可分别回归。
- **一致性约束**：`ChartFixedChild.removable=false` 与 `CurveDefine.h::isFixedChildNode`
  当前一一对应；删除守卫仍以后者为准（命令层全局约束），骨架随类型分化时再统一由
  注册表生成。
- **后续挂载点**：kChartType 切换如伴随结构差异（某类型骨架/初值不同），在命令 redo
  内做"骨架协调"（对照新 profile 补建/移除固定子+初值对齐，包同一撤销宏，用户节点
  一律不动）；详案见桌面《曲线树按图表类型分组重构方案.md》v1.1（§5 渲染行为节、
  §6 骨架协调）。

## 5.9 复数曲线合并为单节点 + Phase 轴（设计图 Complex Plot，本轮已实现）

复数图 MAG/PHA 不再是两条按命名约定配对的曲线节点：导入/定义曲线在复数图目标下
创建**单条复数曲线节点**（树挂 x/ym/yp 三壳），配对由渲染层的名字约定改为模型的
`series.isComplex()` 分派。引擎 PlotView 配套小改（AutoFit 开关 + 下面板标题覆盖口），
重建安装后生效。

- **模型**（CurveDefine.h/CurveDocument/mCurveDocController）：
  - `CurveSeriesData` 新增 `complexYData`（下面板数据，X 共用 xData）、`complexRef`
    （yp 壳编辑的相位侧变换/表达式）、`complexFormat`（0 幅相/1 实虚）、
    `complexSide*` 四来源字段、`isComplex()`；新节点类型 `CurveY2Data`（"yp" 壳，
    固定子不可删，属性按侧重定向 complexRef/complexSide*）。
  - `createComplexCurve`（曲线 + x/"ym"/"yp" 三壳，壳名即树显示名，
    displayOverride 侧别判定退场）；控制器 `addComplexCurve`（命令变体）、
    `setComplexSeriesData`（a/b 截齐兜底直写）、`setComplexSeriesSource`
    （五元组 + ySide*=a 侧 + complexSide*=b 侧）。
  - **kChartType 切换骨架协调**（controller reconcileChartSkeleton，切型宏内）：
    固定子按新旧类型声明补删（进入复数补 YAxis2"Phase"，初值复制主轴、标题
    默认 "Phase"；退出删除）、尾部区顺序对齐声明（复数=Magnitude、Phase、X1，
    新增 CurveMoveChildCommand 同层行移动）、图表名仍为旧类型默认名时随类型
    改名（XY Plot → Complex Plot，用户改过名不动）。切型清空曲线的现状保持。
- **ChartTypeProfile**：复数 profile fixedChildren 重排 {Legend, Y1(Magnitude),
  Y2(Phase), X1, Header, Footer} + `defaultChartName`（"Complex Plot"）。
- **创建链路**：mOpenCurveDlg 追加复数图（特征对按键分组合并为单曲线，显示名
  ="对象 - 方向"，缺对侧降级普通单曲线）；mDefineCurveDlg 复数目标单节点化
  （数学来源表达式落模 ym 壳=幅值式/yp 壳=相位式）；"更改曲线"复数曲线任一侧
  整线换源走成对装配（`siblingComplexComponent` 同方向令牌互换，两路同块装配）。
- **渲染**（mCurveChartView）：`applyComplexSeries` 按 isComplex 分派（复数节点
  → updateComplexCurveData，R23 起引擎键=视图层唯一化键，无碰撞=名（见 §1.2）；
  普通曲线走上面板常规路径），
  `transformedComplexBSeries`（b 侧 complexRef 预变换 + Phase 轴刻度/计权 + 隐藏
  区间同掩码 NaN）；classifyComplexName 跨节点配对/对键反查/下侧 fit 排除整体
  退役；命中/高亮经引擎键（keyFor）反查节点（下面板 getLowerCurve 联动保留，判据 isComplex）。
- **Phase 轴下面板接线**：`applySecondaryAxes` 复数形态改道——Phase 轴不占引擎
  右轴，经 `applyPhaseAxis` 下发下面板 Y（标题走引擎新覆盖口 `setLowerAxisTitle-
  Override`，防 refreshComplexPanelAxes 格式派生标题"相位 (°)"冲掉；可见/刻度数字
  字体/刻度引擎/格式精度/范围全按 Phase 节点属性）；手动范围=关引擎自动适配
  （新 `setLowerAxisAutoFit`）+ setAxisScale，自动=恢复引擎满幅；
  `displayedAxisRange` 复数下 YAxis2 回读下面板当前范围（Phase 页最小/最大回显
  所见即所得）。
- **属性页/树**：mCurveDataPropPage 三态侧别（0=X/1=Y/2=yp，坐标轴行 yp 侧禁用；
  轴下拉项文本改显示名，Y2 显示 "Phase"）；CurveY2Data 注册参数栏页（弹窗回退
  同旧行为）；曲线筛选集合含 CurveY2Data。
- **编辑曲线**：裁剪/删除按索引同步 complexYData；撤回快照（CurveState）扩
  复数数据/格式/complexSide 来源，恢复走复数写口；隐藏区间天然两面板同掩码。
- **边界**：XY 图的 MAG/PHA 仍为两条独立曲线（仅复数图目标合并）；普通曲线切型
  不自动转复数（切型清空重导入）；复数对 X 不一致重采样路径随配对退役（同块
  装配天然同 X）；图例/吸附/十字光标/注释吸附仍仅上面板（引擎 v1 语义未变）。

## 5.10 复数图显示格式切换与面板互换（设计图01，本轮已实现）

- **模型**：Chart 节点新增 `complexDisplayFormat`（0 幅值-相位/1 实部-虚部）与
  `complexSwapped`（上下面板互换）两状态（kComplexDisplayFormat/kComplexSwapped，
  走命令栈可撤销）。节点仍存创建时的原始两路数据（complexYData + complexFormat），
  切换是渲染层现算换算（幅相↔实虚互转：实=幅·cos(相)、虚=幅·sin(相)；幅=hypot(实,虚)、
  相=atan2(虚,实)），模型不动、来回切换无损。
- **入口**（仅复数图可见）：图表空白右键菜单 + 树图表右键菜单（complex profile
  的 Chart menus 追加）——"切换至实部 / 虚部"（文案随当前格式动态）/ "互换复数
  坐标轴"（上下面板交换）。
- **渲染**：`applyComplexSeries`/`refreshComplexCurve` 按图表显示格式换算 a/b
  （`convertComplexDisplay`）并按互换态交换上下面板数据（宿主侧数据交换，引擎
  零改动）；引擎展开方式随图表格式。
- **Phase 轴标题联动**：格式切换时 Phase 轴标题仍为出厂默认（"Phase"/"Imaginary"）
  则随格式联动，用户改过名不动（同 Complex Plot 默认名先例）。
- **互换态 Y 适配**：上面板 Y 自动范围按显示数据（相位/虚部，Phase 轴刻度/计权）
  计算（fitChartAxes 的 fitTopDisp 分支）。

## 5.11 柱状图树：X 轴显示名 "Category"（类目轴）+ 曲线仅 y 壳（本轮已实现）

- **柱状图（chartType=1）图表级 X 轴树显示名改为 "Category"**（柱状 profile
  fixedChildren displayName，对齐参考软件柱状树的类目轴语义；本名仍 "X1"，
  柱状图 X 刻度取类别名的渲染语义不变）。
- **柱状图曲线子节点仅 "y" 一个数据壳**（`createBarCurve`/控制器 `addBarCurve`，
  CurveAddNodeCommand 曲线创建 kind 0 普通 x/y 两壳 / 1 复数 x/ym/yp 三壳 /
  2 柱状仅 y 壳）：柱状图无横坐标数据语义，横轴为类目轴（刻度名存
  series.xTickLabels，数据仍在 series.xData/yData，渲染不受壳影响）。
  创建入口：定义曲线弹窗柱状目标、打开曲线"使用当前绘图"柱状目标、树右键
  "新建曲线"（柱状图表）；曲线右键"编辑数据属性"无 x 壳时回退定位 y 壳。
  参考软件柱状树 Curve 下的 Legend/Category/Y1 子项对应本树已有的图表级节点
  （Legend=图例、Category=类目轴、Y1=值轴），不在曲线下重复挂载。
- **顺带修复**：会话快照 readNode 的类型上界校验停在 CurveYData（CurveY2Data 起
  即被拒、复数会话恢复失败），放宽到词表末枚举（新增类型需同步更新）。

## 5.12 曲线统计——超限频段统计与导出（本轮已实现）

Ribbon"曲线"页 -> "曲线统计"按钮 -> `mCurveStatisticsDlg`（单例无边框白卡片：
曲线形式 动刚度/VTF/NTF 单选 + 频率范围 min—max 两输入框 + 关闭/保存）。旧
"整段特征值统计"（点数/最小/最大/平均/RMS 表）已**替换**为超限频段统计
（20261029 需求，设计稿 design/curve-statistics/ 5 张）：

- **打标约定**：实测曲线与目标值曲线由生成功能创建时打 `CurveCategory` 标记
  （动刚度/VTF/NTF 对话框 slot_create：实测=DynamicStiffness/VTF/NTF、目标线=
  TargetCurve，经控制器 `setCurveCategory` 走命令栈，随生成撤销宏整体撤销/
  重做；导入/定义曲线恒为 Normal 不打标）。序列化按 int 存（V2 曲线节点与
  模板块，读取 clamp 上界=词表末枚举），**TargetCurve 只能尾部追加**。
- **旧曲线限制**：升级前生成的旧曲线无类别标记、统计不到，属预期，不做启发
  式兼容——重新生成一次即可（需求已确认）。
- **统计语义**：逐条实测曲线（visible、xData/yData 非空）与**同图表内**
  TargetCurve 曲线配对（多条目标取第一条并告警；无目标跳过并计数）；实测点
  频率在 [min,max] 闭区间（留空侧不限制）、非"编辑曲线"隐藏区间、x/y 有限才
  参与；目标值按实测点频率在目标曲线上**线性插值**（目标参与采样同样剔除
  隐藏/非有限点，实测点频率在目标范围外跳过）。目标曲线**不校验 visible**：
  隐藏的目标线仍参与配对（隐藏属视图操作，参考数据仍有效；与实测曲线
  visible 即跳过的不对称系有意约定，2026-11-03 裁定）。连续超限采样点合并为一个
  超限频段（单点自成段 f1=f2，相等/非参与点断段），每段输出
  `(f1-f2)-(f极值@极值)` 单元格（数值 'g'6；极值同值取先出现频率）。
- **超限方向自来源结果类型判定**（实测曲线 series.schema，PchReader::toString
  同源）：动刚度+DISPLACEMENTS（位移形式）**低于**目标为超限、段内取最小值；
  其余（速度 VELOCITIES/加速度 ACCELERATIONS、VTF、NTF）**高于**目标为超限、
  段内取最大值；schema 缺失/不可识别的曲线跳过并在导出成功提示附注计数
  （形式不单独持久化）。
- **导出布局**（严格对齐设计稿 01-05 图面，2026-11-02 按需求图复核收紧）：
  动刚度=**行式**（设计稿01-03）：第 1 行=工况角标+合并标题
  `<频率下限>-<频率上限>Hz动刚度统计结果`（输入框留空侧取参与统计数据的实际
  最小/最大频率，全空同样取实际范围；数据宽 <3 列不合并），数据行自第 2 行起
  = 工况列 `Subcase 1 (1008_Z)`（来源五元组 subcase+(对象号_方向) 拼装，对象号
  取 "Point id 1008" 尾 token、方向取 "MAG|Z-Trans" 的 "Z"；subcase 尾部自带的
  pch 激励描述括号段经 normalizeSubcase 剥离（如 "Subcase 101 (1001:+X<>…)"
  -> "Subcase 101"，贴需求图样例；无尾组/嵌套/剥空保持原名，激励由子工况号
  区分）；来源缺失退化为曲线名）+ 逐段单元格（列数=最大段数+1），**无列头行、
  无汇总列**，无超限的
  曲线保留空行；VTF/NTF=**矩阵**（设计稿04-05）：**无标题行**，第 1 行=工况
  角标+对象号尾 token **分组合并表头**（组宽=该组各行最大频段数，下取 1），
  数据行=工况+(方向)（如 `Subcase 1 (X)`）+ 各组子列按序放频段、空缺留白，
  行/列均按文档遍历序首次出现定序，**无汇总列**。
- **导出机制**：宿主组装 `StatsExportTable`（title/layout/corner/groups/spans/
  rows/note，layout=rows/matrix，契约见 export_stats.py 头注释）写临时 JSON，
  QProcess 调程序目录 python 执行 `resource/scripts/export_stats.py`（纯标准库
  手写 OOXML：xlsx 行式=角标+合并标题行、矩阵=分组合并表头，全表细边框样式、
  表头/标题粗体居中、列宽工况 45/频段 24（默认 8.43 会截断），形如数字的单元格
  按数值写出；pptx 行式=标题文本框+纯数据表（无表头行，firstRow 样式关闭），
  矩阵=分组合并表头（gridSpan/hMerge）+数据行、无标题文本框；qrc 打包、程序
  目录缺磁盘副本时 qrc 释放（临时副本每次删除重释放，防升级后旧临时脚本续用），
  失败不留半成品文件——与报告导出 export_pptx.py
  同一机制）。统计只读模型（节点遍历轻量内存运算，不写文档不做文件 IO）。

## 5.13 3D曲线——主页"3D曲线"模块（Plot3D 渲染；R28 入树改造见 5.13.1，其后为 R23~R27 史料）

### 5.13.1 三维图入树（R28，20261110——推翻 R23"不入树"口径，三维图成为曲线文档一等公民）

**树结构**（用户定稿）：三维图表节点（chartType=**5**，固定骨架仅 **Legend**——
因三维图特殊性**无 X1/Y1/Header/Footer**）→ 曲线节点（固定子 **x/y/z** 三个轴壳，
替代 XY 曲线的 x/y 数据壳）。图例部分与 XY 相同（Legend 节点与既有属性页原样）。

- **模型**（CurveDefine/CurveDocument）：
  - 新节点类型 `AxisX3DData/AxisY3DData/AxisZ3DData`（树名 x/y/z，固定子不可删
    不可重命名、无右键菜单；icon=纯√无字形，同 XY 壳）；轴状态**按图表唯一**
    存 `CurveChartData.axis3d`（`CurveChart3DData`，三轴各 label/min/max/
    tickCount/gridVisible/autoFit）——多曲线共享同轴组，**壳读写经文档重定向
    跨两级（壳→曲线→图表）**（Header/Footer 重定向先例），同图表各曲线的壳
    互为同一轴的视图。
  - `CurveSeriesData` 扩 `zData/plot3dType`（0 瀑布/1 曲线/2 曲面/3 柱状，对齐
    弹窗单选序）与 `plot3dRows/plot3dCols`（曲面/瀑布网格行列——**必须持久化**，
    否则会话恢复后栅格形状丢失退化点序）。
  - `chartTypeDisplayNames` 尾部追加"三维图"（第 6 项）；图表属性页类型下拉
    沿用"下标≥3 置灰"既有循环自动覆盖——**既有图表 kChartType 切入 3D 不支持**
    （曲线无 zData 语义），三维图仅经弹窗创建。
  - 序列化：会话快照 **V3**（Chart 追加 axis3d、Curve 追加 zData/plot3dType/
    rows/cols）、模板 **V2**（同款扩展）；节点类型上界校验（readNode/
    readTemplateNode 两处）更新至 AxisZ3DData。
- **ChartTypeProfile**：registry 扩 6 份；3D 份 fixedChildren=[Legend]、
  defaultChartName="3D Plot"、menus={Chart:[显隐/删除], Curve:[删除]}（追加
  曲线仅经弹窗目标选择，树不提供新建曲线入口）、hiddenRibbonActions={NewCurve,
  EditCurve}、**renderKind=Plot3D**（新枚举值）。
- **创建链**（mCreate3DPlotDlg 重构）：解析（m3DPlotDataParser 不变）→ 命令栈
  写文档——新增 `controller::add3DChart(pageId)`（带类型创建命令变体，
  create3DChart）、`add3DCurve(chartId,name)`（CurveAddNodeCommand kind=3）、
  `set3DSeriesData(curveId,x,y,z,plot3dType,rows,cols)`（数据直写，宏外，与
  pch 导入同口径）；**目标选择行**（新建三维图=默认，页定位=当前激活图表所在页
  >最后一页>新建；追加到已有三维图表=下拉列出全部 chartType=5 图表，写前复核
  存活）；撤销宏"新建三维图"/"追加三维曲线"一次整组还原（数据直写不随 redo
  重放，与 pch 导入同源限制）。
- **视口**（视口工厂唯一分发点 materializeChart 按 renderKind 分流）：
  新 `CurveView/m3DChartView`（Qt3DPlotWidget 包装）——文档信号全量重建（整
  widget 重建最简正确）：逐曲线建 Series（线色=模型线色、曲面 Cool/瀑布 Jet
  +Ribbon5/柱状 Rainbow+唯一值点距柱宽，样式迁自 T2 弹窗）→ **`scene()->
  setCoordinateLimits(轴范围)`（多曲线统一按轴范围归一化——引擎序列默认按各自
  数据范围归一化，共轴多曲线必须统一，单曲线=轴范围恰为数据范围观感不变）**→
  axis setLabels/setRange/setTickCounts/setGridVisible（生效四项+自动适配）→
  fitToView。LRU 卸载=视图析构、再物化=重建重放，无损。**GL 纪律**：先 addItem
  再 setData。
- **旧链退役**：`m3DPlotViewportManager.*` 删除；mMechMainWindow 的
  Plot3DViewportIdBase=2000000 段/建卡 lambda/closePlot3DViewport 删除；
  mViewBrowserWindow 删除分流（三维卡片入图表段自动获得 关闭/重命名/激活/会话
  恢复 全链——三维图随文档序列化持久化，.mxses 恢复后按页重建卡片）；
  mMechMainWindow 激活回投/卸载/关闭三处改用工厂新 QWidget 级访问器
  `chartViewWidget(chartId)`（chartView() 的具体类型 cast 对三维视图会漏）。
- **属性生效（T7 前置已就位，页面在 T7）**：生效=轴名称/最小值/最大值/主刻度数/
  显示网格（+自动适配=追加曲线时范围按并集现算）；图2 其余字段置灰占位
  （Plot3D 无对应 API：轴颜色/网格线颜色/字体大小/刻度格式/每刻度网格数/背景
  颜色/坐标轴长度/自动适配之外的范围适配）；图例渲染 R32 已接通（见下方专条）。
  **曲线可见性已接渲染（T8，20261111）**：rebuild 曲线收集按
  `series.visible` 过滤（`collectRenderCurves` 单一收集点，过滤后列表同源
  供并集统计与系列创建——隐藏曲线不建系列、不参与 autoFit 并集/色带 z 范围，
  对齐 2D"fit 只统计可见曲线"口径），`queryAxis3DDisplayRange` 同口径镜像
  （属性页 autoFit 回显=渲染取景），`rebuildRelevant` 后代白名单追加
  kVisible（取消勾选即时重建）；全部隐藏走既有"无曲线"空路径，无特判分支。
  **T7 页面已落地（R31，20261111）**：三壳注册参数栏页 `mAxis3DPropPage`
  （一页类按构造参轴下标 0/1/2 三实例，仿 mCurveDataPropPage 先例；弹窗族
  不注册——壳无双击/右键弹窗入口，showEditorPopup 回退参数栏页=预期），
  **行集口径两次拍板留痕**：首拍 20261111"只放生效项"（3 组 7 行、无置灰
  占位行）已被同日二次拍板推翻——"将图中内容全填入，（不用实现该部分内容）"，
  现行集=参考截图四组 20 行（序=截图）：坐标轴与范围[坐标轴(置灰下拉,回显
  壳身份 X轴/Y轴/Z轴)/应用到(占位)/轴名称(生效)/刻度类型(占位)/最小值/最大值
  (一行双控件容器,生效,互钳与 autoFit 置灰联动保留)/自动适配+适配范围(一行
  双勾选,前者生效后者占位)]、刻度与文字[刻度方式(占位)/主刻度数(生效,
  [2,100] 与渲染端 qMax(2,值) 同口径)/刻度格式(占位,一行双控件)/轴颜色
  (占位)/每刻度网格数(占位)/字体大小(占位)]、网格与外观[网格样式(占位)/
  网格线颜色(占位)/背景颜色(占位)/显示网格(生效,整行勾选)]、坐标轴长度
  [手动轴长度/X轴长度/Y轴长度/Z轴长度(四行全占位,三壳页同构恒显三行)]。
  占位 14 行对齐"极坐标图置灰占位"先例：构造即禁用固定、不连 commit/信号、
  不进 loadFromModel（后续迭代接活 Plot3D API 再翻活）。自动适配交互对齐
  2D"退出自动回写现值"策略：取消勾选
  时先查当前生效范围回填 min/max 并与 autoFit=false 一并提交（视图无跳变、
  手动起点=当前所见；查询失败只退出自动、回显模型休眠值），勾选时最小/
  最大值置灰并回显并集现值——范围查询走 `m3DChartView::
  queryAxis3DDisplayRange`（静态、纯文档计算不依赖视口；`unionAxisRange`
  单一实现两处共用，rebuild 渲染行为零变化）；同图表曲线增删/数据直写经
  构造函数全局监听刷新置灰回显（本节点自身变化基类已回填，不重入）。
  回显全程阻塞信号（R30 教训）。
- **图例渲染（R32，20261111——宿主侧自绘覆盖层，"Legend 页就位不接 3D 渲染"
  旧口径作废；用户拍板：宿主自绘叠加不动 Plot3D 引擎/自动定位=固定图内右上
  不做曲线遮挡采样/仅显示不做画布交互）**：`CurveView/m3DChartLegendOverlay`
  （QWidget 子控件叠于 Qt3DPlotWidget 之上——视图子控件非画布子控件，rebuild
  deleteLater 重建画布无需随建随销，几何/层级由 resizeEvent 与 rebuild 尾
  raise 同步；`WA_TransparentForMouseEvents` 鼠标穿透，图例区域下拖拽仍旋转
  视角；背身透明不挡 GL 画布）。数据**零模型改动**只读 `CurveLegendData`：
  条目=本图表渲染曲线（树序，与 rebuild 的 `collectRenderCurves` 同一收集
  口径 series.visible 过滤——隐藏曲线不建系列也不进图例，所见即所得），每
  条目=线样图标（series.color 短线段，线宽 1.5 与 3D 曲线渲染同值）+曲线名
  （节点本名，3D 无标签系统不用 legendText），文本=lg.font 加粗对齐 2D
  applyLegend 口径；观感逐项对齐引擎 mPlotLegendItem（布局常量 margin15/
  padding10/itemGap6/iconWidth35/textOffset8 同值；边框 0无/1实线/2虚线/
  3划线 CustomDashLine/4点划线，borderWidth>0 且 borderStyle==0 按实线兜底
  =2D `qMax(1,borderStyle)` 口径；背景=lg.bgColor 含 alpha 默认白半透明；
  文本色两态=textColorMode 0 跟随曲线色/1 统一自定义；引线位置 0 图标在
  文字左侧/1 下方）。位置（paintEvent 内现算）：autoPosition 或 position=
  2/5~8（图外四向与 2D 同口径映回右上）→ 图内右上角，**色带避让**=图例矩形
  与引擎 `getColormap()->getLegendRect()`（宿主传入，矩形无效=色带未画跳过）
  相交时水平左移留约 8px（色带默认贴右上 m_relPos=1.0,0.0；手动位置 0/1/3/4
  不避让）；position=0/1/3=图内左上/左下/右下角；4=自定义（posX/posY 画布
  百分比、框左上角锚点，语义对齐 2D setFloatingPos，越界收敛）。图例节点
  缺失/无条目/visible=false 整框隐藏不崩溃。**刷新分流（关键体验项；评审 R1
  修正分立结构）**：`legendPropRelevant` = ①本图表 **Legend 节点**×12 项参数
  栏键（kVisible/kAutoPosition/kPosition/kPosX/kPosY/kLegendBorderWidth/
  kBorderColor/kLegendFont/kLeaderPos/kBgColor/kTextColorMode/kTextColor；
  kPosX/kPosY 与注释侧同名按节点类型区分）∨ ②本图表 **Curve 节点**×kName
  （图例条目文本=本名的同步；两分支互不放行对方键）→ 命中仅调
  `refreshLegend()` 刷覆盖层，**禁止整场景重建**（改图例颜色不闪重建、相机
  状态不丢；内部脏键比对，画布每帧 Paint 经 eventFilter Queued 同步色带矩形
  ——新建画布首帧避让即正确——零变化免重绘）；曲线增删/数据直写/轴键仍
  scheduleRebuild 整场景重建，重建尾覆盖层随 rebuild 重放（新建视图首显即有
  图例）。撤销/重做走既有 CurveSetPropertyCommand（零新增模型/命令），
  .mxses 恢复后图例随文档重放。无画布交互（仅显示，编辑入口=树选 Legend
  节点→参数栏 mLegendPropPage，现状已通，页面零改动）。
  回归：workspace `tools/3dtest/tst_3dchain` L 项（首显即有图例/默认右上+
  色带避让/曲线色条目像素/图例键提交画布实例与相机矩阵不变/kVisible 整框
  隐藏+undo 恢复/手动位置不避让/引线下方加高/边框线宽 0=无框/改名信号链
  自然重绘计数+同名提交负向对照+盒宽增长内容断言——评审 R1 加固：改名断言
  改走信号链自然刷新路径观察，不用 grab() 强制同步重绘以免掩盖断链）。
- **边界**：定义曲线/编辑曲线/基准线/应用样式/模板曲线/曲线统计/报告导出
  均不适配三维图；**注释=R34 起界面+模型已接（树节点/参数栏三维注释管理页/
  树右键删除/会话序列化），R42 二期起画布渲染与画布管理全链生效（画布注释
  项/点击选中拖拽移动/图表级总开关/剪贴板/模板求值，见 R42 条目）**；
  单文件单曲线（追加经弹窗多曲）；3Dface 类两列对角线文件忽略
  （R24 用户拍板）。
- **化石自愈 + 切入守卫（R29，20261111）**：R28 当日中间开发版曾以"直写
  chartType 绕过 reconcile"产生化石树（chartType=5 + XY 固定骨架 X1/Y1/
  Header/Footer + 旧默认图名 "XY Plot"），被 .mxses 忠实保留。两道收口：
  - **恢复自愈**（CurveDocSerialize.cpp `restoreCurveDocument`，insertNodeTree
    之后执行）：遍历恢复树中全部 chartType=5 图表——删除三维图声明固定骨架
    之外的图表级固定子类型节点（保留集=`chartTypeProfile(5).fixedChildren`
    类型表，骨架集合以既有 `isFixedChildNode` 判定——数据壳挂曲线下不可能是
    图表直属子节点，故判定安全；删除走文档既有直写口 `removeNodeTree`，行信号
    +idMap 摘除+释放与命令栈底层同机制）；图表名仍为旧类型默认名（其余注册类型
    `defaultChartName` 集合={"XY Plot","Bar Plot","Complex Plot"}）时改名
    profile(5).defaultChartName（"3D Plot"），用户改过名不动。曲线（含 x/y/z
    轴壳）/Legend/用户节点/axis3d/数据一律原样；直写不进命令栈（restore 本为
    直写路径）；正常三维图两类内容均不命中，幂等零变化。自愈发生在卡片打开前
    （mxses 恢复中卡片由页重建统一开），树/视图按自愈后文档构建。
  - **kChartType 切入守卫**（mCurveDocController `setNodeProperty`）：
    `prop==kChartType && value==5 && 节点为 Chart` 静默返回不建命令（撤销栈无
    残留）——曲线无 zData 语义、骨架协调补不齐三维语义，该入口是化石的最后一
    个产生向量；0/1/2/3/4 间既有切换不受影响，三维图仅经"从文件新建三维图"
    弹窗创建。
  - 回归：workspace `tools/3dtest/tst_3dchain` E 项（化石 save→clear→restore
    自愈断言；C 项同步为守卫拒绝语义）。
- **回显防毒化 + 弹窗单视口（R30，20261111）**：首建"2D 树+3D 视口"根因=
  图表属性页/弹窗图表页 `loadFromModel` 未阻塞信号回显类型——chartType=5 被
  `qBound(0,5,4)` 钳到 4，combo 首载 0→4 发 `currentIndexChanged` 走真实切换；
  首建流程该加载发生在创建宏中途（图表已建、曲线未挂，hasCurve=false 无确认框
  静默提交）：类型改 4+骨架协调补建 X1/Y1/Header/Footer+默认名随类型改回
  "XY Plot"（树=2D 形态），视口在 nodeAdded（尚为类型 5）时已物化为 3D 视图、
  切型后工厂不换视图类且 `m3DChartView::rebuild` 对 type≠5 早退（视口=不刷新
  空盒）；第二次起 combo 停在 4 档 setCurrentIndex 无变化不再触发，故"第二次
  才正常"。三道收口：
  - **回显信号阻塞**（`mChartPropPage::loadFromModel` 与
    `mChartEditorPropPage::loadFromModel` 两处同改）：类型回显包 QSignalBlocker
    （回显永不触发切换）；chartType=5 回显第 6 项"三维图"（置灰项仅显示，
    顺带修复"选中三维图显示蛛网图"的外观遗留）；其余类型维持 qBound(0,t,4)
    既有钳制。
  - **切换守卫纵深**（`applyChartTypeSwitch` 入口，mCurveEditorWidgets）：
    `modelIdx==5 || idx==5` 一律拒绝——idx≠modelIdx 时 QSignalBlocker 回退
    下拉后 return（无确认框、无曲线清理、零提交），idx==modelIdx（5→5 回显）
    与既有回显短路同口径直接返回。三维图不参与类型切换：切出会破固定骨架/
    默认名（R30 毒化同款形态），切入被 R29 setNodeProperty 守卫拒但下方曲线
    清理已先行=数据丢失路径，一并封死；XY/柱状/复数等既有切换零回归。
  - **弹窗单视口化**（mCreate3DPlotDlg）："目标"行整组 UI（新建/追加单选+
    目标下拉）与 refreshTargets/_targetChartIds/写前复核机制移除，弹窗仅
    数据文件+绘图类型+绘图；slot_plot 收口为：解析成功后查全文档是否已有
    chartType=5 图表（遍历 工程→页→图表，全文档唯一不限定页）——无→既有
    "新建三维图"宏（addPage→add3DChart→add3DCurve，B02 新视图页口径不变）；
    有→"追加三维曲线"宏仅 add3DCurve（追加到该图，曲线名=文件基名，同名
    允许）；用户删除三维图后再绘=重新新建；提示行/成功关窗/红字行为不变。
    存量多三维图会话不合并（用户拍板），追加落首个命中的三维图。
  - 回归：workspace `tools/3dtest/tst_3dchain` G 项（对三维图 applyChartType
    Switch 拒绝零副作用+5→5 回显短路+空 XY 图 0→2 直提不受误伤）；既有中毒图
    （type=4）数据不自动修复，手工删除三维图后重绘即可。
- **画布生命周期卫生（B03，20261111——瀑布/曲面绘制弹 QOpenGLWidget
  "ctx && fbo" 断言修复）**：机制=Qt5.13 QOpenGLWidget 隐藏/失效即释放 FBO
  （context 保留），而 rebuild 的旧画布拆除仅 deleteLater（未 hide 未摘布局），
  deleteLater 窗口期新旧两画布并存参与布局几何，已初始化画布在失效态收到
  resize/paint 命中 qopenglwidget.cpp recreateFbo 族 `Q_ASSERT(ctx && fbo)`
  （qtbase 已知生命周期缺陷族，QTBUG-70636 等；用户复现数据
  3DSURFACE_p1w1s1.csv 151×151）。两道宿主收口（m3DChartView）：
  1) **拆除顺序**：旧画布先 `layout()->removeWidget` + `hide()`（隐藏即同步
     失效——Qt 不再向隐藏 widget 投递 paint/resize、不再参与布局几何）再
     deleteLater；
  2) **不可见跳过重建、showEvent 补建**：rebuild 入口 `!isVisible()` 置
     `_rebuildPending` 早退，`showEvent` 经既有 0ms 合并定时器补跑——杜绝向
     未首显/隐藏画布 addItem/setData/update 与隐藏期无谓几何翻动；可见期渲染
     口径零变化（文档信号全量重建、先 addItem 再 setData 均不回归）。
  引擎侧同日修复（Plot3D 独立仓单独提交）：WaterfallSeries setData/
  setRibbonWidth 补 `m_viewer->context()` 双守卫（对齐 Surface3DSeries 全库
  纪律；缺守卫时无上下文跑 init() 直接触发 `Q_ASSERT(context)`
  qopenglfunctions.cpp:189，离屏 S4/S6 场景实证致命复现）；WaterfallSeries/
  Bar3DSeries init() 补数据兜底重建、Surface3DSeries init() 兜底写法修正
  （Line3DSeries::init 同款裸兜底——兜底不可再 makeCurrent/doneCurrent：
  initializeGL 持有的上下文被 doneCurrent 释放会让后续 paintGL 全部落空，
  即"setData 早于首显"首显空绘的实证根因）。修复后瀑布/曲面/柱状三类
  "setData 早于首显"首显渲染均非空（离屏像素断言：饱和像素
  82785/69063/88895、量化色种 60/185/142）。回归：workspace
  `tools/3dtest/tst_qwa_assert` 8 场景（首建 churn/隐藏期追加+重显/隐藏期
  churn/未显示 churn/混合风暴/新建→追加→页切换→删除→重建全流程/grab 探针×2）
  全过零断言 + tst_p4mirror 宿主时序镜像 + `tst_3dchain` 53 项 +
  `axis3dtest` 75 断言全过；Plot3D 重建安装+头副本同步，bin/Debug 与
  install/bin 及 Basic/Plot3D/install/bin 三处 Plot3Dd.dll/PostCurved.dll
  逐字节一致。
- **三维曲线参数栏页 + 样式增量刷新（R33，20261112）**：三维图（chartType=5）
  下曲线节点选中后参数栏不再回落二维曲线通用页——新页 `CurvePages/
  mCurve3DPropPage` 行集对齐设计图 `design/3d-curve/ref-3dcurve-prop-page.png`
  三组（组名/行名/行序逐项一致，控件=模块参数栏既有件）：
  - **行集**：标准[可见性(生效 kVisible,整行勾选,四类 plot3dType 恒可用)] /
    曲线标签[显示标签/标签内容/显示标签前缀/标签前缀/显示标签后缀/标签后缀
    全占位——标签内容=置灰回显曲线本名（唯一进 loadFromModel 的占位行，只读
    展示不连 commit；本节点 kName 变化经基类 onNodePropertyChanged→
    loadFromModel 自动刷新，零额外接线）] / 外观与着色[颜色(生效 kColor)/
    线宽(生效 kLineWidth，`mCurveLineWidthCombo` 1..4px 图标档，提交=档位px、
    回显就近取整钳 1..4 与二维页同口径)/线型(置灰固定实线——引擎 OpenGL Core
    无点划线能力)/符号(行标签"符号"+行内[组合固定"无"+色块]双控件容器,整行
    置灰)/符号间隔(置灰 1)/
    符号大小(置灰 5)/线下区域填充(置灰,勾选态照设计图)/填充透明度(置灰滑条
    固定 70,无模型键)/云图(行内[勾选框"云图"不勾+组合"x"+"编辑图例..."按钮]
    三控件容器,全占位)]。
    **生效/占位口径（20261112 用户拍板）**：生效 3 行走命令栈可撤销；其余
    全部按 R31 先例占位——构造即禁用固定、不连 commit/信号、不进回填（标签
    内容回显本名例外）；回显全程 QSignalBlocker（R30 教训）。**动态启停**：
    loadFromModel 按本曲线 `series.plot3dType==1` 启停颜色/线宽两行（0 瀑布/
    2 曲面/3 柱状走色带渲染不消费线色/线宽，`setRowEnabled` 连行标签一并
    置灰防"提交无视觉效果"的假编辑）。
  - **注册分流**：`mCurvePropPageManager::takeParamBarPage` 对 Curve 节点经
    `doc->chartNodeId` 查所属图表，chartType==5 路由三维页并用**负变体键
    -typeInt 独立缓存槽**（二维页正键槽不动——同会话二维↔三维曲线来回切换
    两页各自缓存、回显互不串值；三维页是 Curve 节点在三维图语境下的形态而非
    独立节点类型，不走 registerPage，注册表 NodeType→工厂语义保持纯净）；
    chartNodeId 查不到（≤0）或图表节点缺失按二维回落；弹窗族不注册三维页
    （三维画布无双击/右键编辑入口，showEditorPopup 路径不可达，行为不变）；
    挂载前 setNodeId 既有调用序未动。
  - **颜色/线宽生效路径=保视角增量刷新**（用户拍板：改一次颜色视角不再重置
    回默认方位）：`m3DChartView` rebuild 建 plot3dType==1 序列时记录
    `曲线id→std::shared_ptr<Line3DSeries>` 映射（成员 QHash，rebuild 开头
    clear——画布销毁重建映射随场景整体换代，shared_ptr 防悬挂）；
    `setLineWidth(1.5f)` 硬编码退役→`setLineWidth(float(series.lineWidth))`
    （存量三维曲线渲染略变粗 1.5→2.0 属接模预期变化，需求备案）。
    nodePropertyChanged 分流新增**样式分支**（判定序在 legendPropRelevant
    之后、rebuildRelevant 之前；条件=Curve 节点∧本图表∧prop∈{kColor,
    kLineWidth}）：命中即按模型现值 setColor/setLineWidth + `_plot->update()`
    ——引擎两 setter 均为绘制期消费（draw() 内 setUniformValue("color")+
    glLineWidth，改成员下一帧自然生效，无需重建 GPU 缓冲），处理完 return
    不落入 scheduleRebuild（rebuildRelevant 白名单不追加这两键，防双触发）；
    kColor 时另调 `_legendOverlay->refreshLegend(...)` 同步图例。映射无该 id
    （隐藏曲线无序列/色带类序列）安全短路。**不可见期闭合（B03 口径）**：
    视图不可见时增量分支不碰画布，改调 scheduleRebuild（其内部 !_isVisible
    置 _rebuildPending，showEvent 补跑全量重建重放正确样式）——可见期增量
    保视角、隐藏期挂起全量，语义闭合无丢样式窗口。kVisible 既有重建路径、
    轴键、legendPropRelevant 零变化。
  - **图例覆盖层图标线宽跟随**（R32"图标线宽与 3D 曲线渲染同值"语义随移）：
    `m3DChartLegendOverlay` 条目线样图标线宽 1.5 改读本曲线
    `series.lineWidth`（渲染合理范围钳 1..4，与属性页档位同口径），线宽纳入
    脏键（色值本就在键）——线宽提交经"setLineWidth→update→画布 Paint→
    Queued refreshLegend"既有链路自然重绘；隐藏曲线不进条目口径不动。
    Plot3D 引擎零改动（纯宿主侧）；PlotView 不涉及。
  - 回归：workspace `tools/3dtest/tst_curve3dpage`（42 断言：分流两页独立
    缓存/三组行集结构/占位零 commit（撤销栈 count+index 双零探针——push 遇
    已撤销段先截断 count 不增，QUndoStack 标准语义，axis3dtest P7 同款
    备案）/生效行 commit+undo/瀑布页置灰/增量刷新画布实例+相机矩阵不变+
    像素变化/图例图标线宽蓝像素 38→178+自然重绘计数/隐藏曲线提交安全短路/
    不可见期提交挂起→重显补跑）+ `axis3dtest` 75 断言 + `tst_3dchain`
    ALL PASS + `tst_qwa_assert` 9 场景全过 + ctest 其余 7 项一致
    （PostWindowTests/PostWindowIsolation 两失败为基线遗留，stash 验证与
    本单改动无关）。

- **三维图注释管理页 + Ribbon 入口（R34，20261112）**：注释成为三维图下的普通
  树节点（**模型键全既有，零模型改动**——addAnnotation(chartId) 对 chartType=5
  现状可用、注释分区 defaultZoneOrder 已含 Annotation、序列化走既有 V3 快照），
  本轮**仅界面+模型接线**（用户拍板），注释暂不显示在 3D 画布（画布渲染为
  下一期独立需求）。三个落点：
  - **Ribbon 入口**（mPostMenuContributor::build3DCurvePage，PostWindow 单元
    提交）："文件"组右侧 `addGroup("3D曲线", "标注")`+"注释"大按钮（图标=2D
    曲线页注释按钮同款资源，tooltip"添加三维图注释"）。启停联动对齐"定义曲线"
    按钮先例——工厂 sig_currentChartChanged+文档查表，当前激活图表为三维图
    （chartType==5）时启用，否则置灰（含无激活图表 -1）。点击=beginMacro
    （"新建注释"）→ addAnnotation(chartId) → kAttachMode=1（3D 语义=视图类
    "窗口"，默认 0 曲线类不符）→ endMacro → setCurrentNodeId（树回选+参数栏
    挂页全链免费获得），一次 Ctrl+Z 整体撤销新建。
  - **参数栏三维注释管理页**（CurvePages/mAnnotation3DPropPage，新增）：四组
    14 行（组名/行名/行序=参考截图 design/3d-curve/ref-3dcurve-annotation-
    page.png）。注释管理[显示注释(置灰占位，tooltip 注明"图表级总开关下一期")/
    当前注释(生效——下拉列同图表全部注释名树序，选中项切换页面绑定，经选择
    中枢 setCurrentNodeId)/注释列表(生效——QListWidget 逐条目勾选框提交该条目
    kVisible，单击条目=切换页面绑定与下拉联动)/新增(生效，同 Ribbon 点击行为)/
    剪切/复制/粘贴(三钮置灰占位，2D 注释右键"复制置灰占位"同策略——无剪贴板)]/
    文字[文本编辑区(生效 kText——**失焦且文本有变化才 commit 一次**（防逐键压
    撤回栈，QPlainTextEdit 无 editingFinished 经事件过滤器抓 FocusOut，与模型
    现值比对"无变化不发"对齐 mCurveTextButton 口径）/(X)(Y)(Z)(U)(V)(W)(Label)
    七按钮(置灰占位，3D 模板求值随渲染下一期))/对齐(生效 kAlign 0左 1中 2右，
    注释侧语义)]/属性[显示边框(置灰占位)/文字颜色(生效 kColor，单色块按钮照
    截图——不设"默认/自定义"组合，无效色恢复入口随渲染下一期)/字体设置(生效
    kFont，mCurveFontButton 点击弹 2D 同款 QFontDialog"选择字体"——用户拍板
    不单列字体/字号/粗体/斜体四行)/背景颜色(生效 kBgColor，同文字颜色口径)/
    边框线宽度(生效 kBorderWidth，1..4px 图标档，提交=档位px、回显就近取整钳
    1..4，与 2D 注释页同映射)]/附着对象[附着方式(置灰回显"窗口"——创建时已
    提交 =1，3D 无第二附着语义，按占位行纪律)]。占位纪律 R31/R33 同款逐条
    （构造即禁用固定、不连 commit/信号、不进回填；QPushButton 置灰观感自身
    样式表兜底——FoldSection :disabled 规则只覆盖 Combo/Line/Spin），生效行
    回显全程 QSignalBlocker（R30 教训）。列表/下拉内容=同图表（父 chartId）
    下树序全部 Annotation 兄弟节点：本节点 kName 变化经基类
    onNodePropertyChanged→loadFromModel 自动刷新；兄弟增删/改名/显隐经构造
    函数全局监听刷新（mAxis3DPropPage 先例：全局监听+本节点自身变化不重入）；
    绑定节点被删除时页面随既有 sig_nodeRemoved 链路自动关闭/解绑（既有行为）。
  - **注册分流**（mCurvePropPageManager::takeParamBarPage）：Annotation 节点
    经 chartNodeId 查父图表，chartType==5 路由三维页并用**负键 -typeInt 独立
    缓存槽**（守卫/回落口径逐条对齐 R33 Curve 分流：chartNodeId 查不到/非
    Chart 按二维回落，二维正键槽与 2D 注释页零回归；弹窗族不注册三维页——
    三维画布无双击/右键弹窗入口，showEditorPopup 回退路径行为不变）；
    ChartTypeProfile d3 份 menus 增 Annotation={RemoveNode}（对齐 2D 注释菜单
    仅删除口径；**不加 AddAnnotation**——入口在 Ribbon 按钮，对齐"追加曲线
    仅经弹窗"先例；注释插入位置由既有 defaultZoneOrder 注释分区决定零改动）。
  - 验收边界：注释节点随 .mxses 保存/恢复（V3 通用序列化，零新增）；树右键
    注释节点=仅"删除"（走命令栈）；2D 图表下选中注释仍走旧 mAnnotationPropPage
    （负键分流不误伤）；文本编辑区失焦才提交、无变化零提交（连点两次无中间
    提交）。

- **三维图删除曲线场景残留修复（R35，20261112）**：树右键"删除曲线"后树行与
  图例条目正常消失，但 3D 场景仍渲染已删曲线/曲面/柱——`m3DChartView::
  rebuildRelevant` 判定序缺陷：`chartNodeId(nodeId) != _chartId` 守卫先于
  `prop.isEmpty()` 广谱分支，而删除通知（sig_nodeRemoved，prop 空）到达时节点
  已从文档移除、`chartNodeId` 反查失效恒返 -1，守卫一律 `return false` 吞掉
  通知→scheduleRebuild 永不触发→旧画布（整画布换代重建机制）原样保留；树
  （模型行移除信号）与图例覆盖层（Paint 期拉取文档现状）不依赖该判定，故表现
  正常，恰成"树/图例清了、场景没清"的分裂观感。
  - **修复**：结构类通知（prop 空）分流提到 chartNodeId 守卫之前——节点已
    不可查（`!doc->node(nodeId)`）=删除侧，广谱接受返回 true（对齐 2D
    `mCurveChartView::onNodeRemoved`"nodeRemoved 时节点已不可查→保守全量
    重建"既有口径）；节点可查=新增/撤销恢复侧，仍按
    `chartNodeId(nodeId)==_chartId` 精确过滤。属性类通知（prop 非空）路径
    零变化（守卫后移不改变其判定序：先归属过滤再键白名单）。
  - **波及面备案**：广谱接受使文档任意节点删除都会触发各 3D 视图重建——删除
    为低频用户操作、rebuild 仍有 0ms 合并与 B03 不可见挂起兜底，可接受；注释
    /基准线等非曲线删除同步进重建（3D 注释画布渲染上线后属正确行为，当前
    重建重放零变化）；Chart/Page 删除仍由工厂 aboutToBeRemoved 关卡片（
    rebuild 对已删图表查表早退，双路径不打架）；撤销恢复走 nodeAdded 可查
    路径精确过滤不受影响。
  - 验证：PostCurve 增量编译通过（m3DChartView.cpp 重编入 PostCurved.dll）；
    行为验证=右键删除曲线/曲面后场景即时清空、Ctrl+Z 恢复后场景重现。

- **三维曲线"外观与着色"七行翻活（R36，20261112）**：R33 占位的 线型/符号/
  符号间隔/符号大小/线下区域填充/填充透明度/云图 七行接成真实生效功能
  （开发=项目经理亲執：子代理配额 20261112 两次 5h/7d 封顶，R28 先例；阶段 0
  由派发 subagent 完成并提交 d4520a77）。两仓落点：
  - **引擎（Plot3D 独立仓 wyf_dev，c03f0a8）Line3DSeries 四能力**（setter
    默认全关闭零回归）：
    ①线型=顶点累计弧长属性+片段 discard（OpenGL Core 无 glLineStipple），
    图案周期为归一化盒空间常量（0实/1虚/2点/3点划）；②符号=球/立方体/
    八面体三网格实例化渲染（样式 4~9 回落球体=已知限制备案），间隔 N 取样/
    独立符号色/世界缩放；③线下区域填充=相邻点对向 Z 最小值地板拉幕帘
    （不透明度语义 alpha=值/100 默认 70），半透明独立混合绘制（不写深度、
    先于线体/符号）；④云图=线体与符号按所选分量原始值采样色带纹理渐变，
    值域经 bindColormap 同源 widget 级 Colormap 的 u_MinZ/u_MaxZ 单一真源。
    **setCoordinateLimits 覆写标脏**，draw 前按当前范围重建（弧长/幕帘地板
    缓存与轴范围保持一致）；样式 setter 均为成员写+标脏，draw 期消费无场景
    重建。**教训备案：vec3 uniform 勿用 setUniformValue(QColor) 直传**——
    Qt 该重载走 vec4 通道，类型不符被 GL 静默丢弃（符号一度全黑）。
  - **宿主**：模型三新键 kAreaFillOpacity(0..100 缺省 70)/kContour/
    kContourVar(0=x/1=y/2=z) + CurveSeriesData 同名字段（阶段 0，d4520a77）；
    线型/符号四件/区域填充复用既有键零新增；**会话快照 V3→V4**（Curve 节点
    追写三新键，读侧版本门控+越界钳制，旧 .mxses 缺省兜底零损伤；模板路径
    writeTemplateNode/readTemplateNode 独立未动）；mCurve3DPropPage 七行
    翻活（回显 QSignalBlocker，"编辑图例..."钮恒置灰占位=色带图例编辑
    下一期）；**m3DChartView 增量样式分支键集从 {kColor,kLineWidth} 扩至
    外观与着色全键**（判定序仍在 legendPropRelevant 之后/rebuildRelevant
    之前，命中即 return，白名单不追加防双触发；rebuild 建序列时按模型现值
    全量下发，符号大小=模型 1..100×0.03 世界缩放）；**色带值域云图优先**：
    图内有云图开启的可见曲线时 setDataRange=其分量值域（contourValueRange
    单一实现 rebuild 与增量共用），否则既有 z 并集口径；混合变量云图曲线
    不并入（一条色带只表达一个维度，已知限制备案）。
  - **默认值备案**：symbolSize 模型缺省 6（设计图示 5 属截图态）、areaFill
    缺省 false（设计图勾选态属截图态）、填充透明度缺省 70 对齐设计图滑块
    位置、云图变量默认 x。
  - 回归：workspace `tools/3dtest/tst_r36` 新增 48 断言（页七行
    commit+undo/占位钮零 commit/瀑布九行整组置灰/V3 旧快照容错/V4 十键
    往返/增量刷新画布实例+相机矩阵不变+线型/符号/填充/云图像素断言/隐藏期
    挂起重显补跑）；tst_curve3dpage 42 断言与 tst_r33qa 15 断言中 3 条
    R33 旧占位口径断言更新为 R36 生效口径（S3c 占位恰 7/S3d 普查 3/0/2/4/
    Q1e 出厂值钉住）；3dtest 全套+axis3dtest 75 断言+tst_qwa 9 场景+
    ctest 基线一致（两失败系基线遗留）；**engine ABI 变更后 workspace 旧
    exe 必须重建**（axis3dtest 旧 exe 加载新 DLL 曾段错误，重建即愈）；
    Plot3Dd.dll 三处逐字节一致（Plot3D install/bin、bin/Debug、install/bin）。

- **3D曲线工作台 Ribbon 页与参数栏工具页补齐（R37，20261112）**：Ribbon
  "3D曲线"页签按设计稿重组为五组九钮并实现"点击按钮→参数栏切换"——
  文件[新建]/曲线[定义曲线, 瀑布图]/格式[曲线样式, 坐标轴]/查询[数值,
  数据查询]/标注[基准, 注释]（设计稿 design/3d-curve/ref-ribbon-3dcurve-
  tab-full.png 与五张参数栏页设计稿，用户 20261112 提供；两处口径拍板=
  曲线样式页接既有功能、缺图标暂复用现有资源）。新建/定义曲线/注释三钮
  行为原样（定义曲线仅自文件组移入曲线组，弹窗 mDefine3DCurveDlg 既有）。
  四个落点：
  - **工具页五件**（CurvePages/mWaterfall3DPage/mCurveStyle3DPage/
    mPointSeek3DPage/mSectionQuery3DPage/mDatum3DPage，**非节点绑定页**——
    不继承 mCurvePropPageBase、不进 mCurvePropPageManager 注册表，页顶加粗
    标题+FoldSection 分组）：瀑布图/数值/数据查询/基准四页=纯展示占位
    （用户拍板"没有的功能先不实现"——R31 占位纪律逐条：构造即禁用、不连
    commit/信号、不进回填，控件值=设计稿截图值静态成型，表格/列表留空
    不造假数据）；曲线样式页=图表级新页接既有功能（见下）。窄宽自适配
    B01/B02 纪律（按钮 minimumWidth(1)+紧凑内边距，实测五页
    minimumSizeHint 宽均 142px，窄 dock ~316px 无裁切）。
  - **曲线样式页接线**（图表级，挂载前经 setChart(factory->currentChartId())
    绑定）：图形类型行=**类型视图下拉过滤器**（20261117 勘正：原置灰占位
    "曲线"改与 mDefine3DCurveDlg 类型视图下拉同款——曲线/曲面/瀑布图/柱状图
    四项 kTypePlot3d 同表，恒可用不随选中态/图表启停，切换=按 plot3dType
    重建过滤列表并自动选中首条、无条目无选中态，纯视图态不回写模型）；
    曲线列表=本图表**当前类型视图** Curve 子节点树序（**ExtendedSelection
    多选**——20261117 二次勘正：多选同调样式，选中集跨刷新保持；勾选态=
    series.visible 回显，用户勾选=即时提交该曲线 kVisible
    单命令）；按钮行1 全部/取消/反选/已显示=**列表选中集纯 UI 操作**
    （20261117 勘正：原勾选态操作改选中集——全部=一键全选/取消=清空选中/
    反选=翻转/已显示=按模型 visible 仅选可见者）；按钮行2
    显示/隐藏=**选中集**批量提交 kVisible（beginMacro"曲线样式-显示/隐藏"
    一次 Ctrl+Z 整组撤销，空选中集零操作；原勾选集口径下"显示"恒空集无
    作用——被隐藏曲线必未勾选进不了勾选集，改选中集后选中之即可再显示），
    图例显示/图例隐藏=查本图表
    Legend 节点提交 kVisible（R32 覆盖层消费）；外观与着色=**两套行集按图形
    类型视图二分**（20261117 三次勘正：切类型外观组随变，R38 口径自
    mCurve3DPropPage 镜像 updateAppearanceRows）——曲线（==1）九行**作用于
    选中集全部曲线**（回显锚=当前行优先否则选中集首行；空选中整组禁用；
    多条提交 beginMacro"曲线样式-批量修改"一次 Ctrl+Z、单条仍单命令），
    键集/控件/回显钳制逐项对齐 mCurve3DPropPage R36 现状（kColor/kLineWidth/
    kLineStyle/kSymbolStyle/kSymbolColor/kSymbolDensity/kSymbolSize/
    kAreaFill/kAreaFillOpacity/kContour/kContourVar，"编辑图例..."恒置灰
    =下一期）；色带类=曲面/瀑布（2/0）四行[显示模式（kDrawStyle 三选项，
    仅曲面视图生效、connect 带类型守卫）/颜色（bar 形态置灰静态色：瀑布=蓝
    85,116,189/曲面=橙 221,147,96/柱状=紫 145,112,188）/不透明度（置灰静态
    滑条：瀑布 65/曲面 50）/云图（三控件容器全置灰）]、柱状（3）仅颜色一行
    （除曲面显示模式外占位照设计稿，占位纪律同 R38）；可见性切换=FoldSection
    setRowVisible 行摘除/插回，全部走 controller setNodeProperty 命令栈
    （颜色/线宽经
    R33 保视角增量刷新生效，零新增渲染代码）。上下文维护=构造函数全局
    监听控制器三信号（mAnnotation3DPropPage 先例）：本图表后代增删/属性
    变化即重载（回显全程阻塞信号幂等收敛），图表删除→列表清空全钮禁用
    不崩溃、重绑恢复。
  - **Ribbon 挂载机制**（mPostMenuContributor，PostWindow→PostCurve 既有
    依赖方向）：五工具钮点击=unEditInfoWindow → activateDock("Info") →
    窄宽压缩（mPostInfoHub::mountToHost 同款"只降不升"）→
    IPanelHost::setMainWidget(page)——**页面懒创建 QPointer 成员缓存复用**
    （mInfoWindow 换页对旧页只 hide+setParent(nullptr) 不删除，缓存保活是
    硬要求；重复点击同实例重挂，曲线样式页重挂前 setChart 刷新上下文）；
    坐标轴钮点击=选中当前三维图**首条曲线（树序）的 AxisX3DData 固定子**
    （setCurrentNodeId 走既有 sig_currentNodeChanged→editCurveNode 链路挂
    mAxis3DPropPage，零新页面；无曲线/壳静默返回）；六新钮统一 chartType==5
    启停联动（R34 注释钮同款：sig_currentChartChanged+查表，含 -1）。
    **不新增 qrc 资源**（缺图标六钮暂复用：瀑布图→结构树筛选/绘图、曲线
    样式→操作/应用样式、坐标轴→结构树筛选/坐标轴、数值→操作/选取、数据
    查询→操作/后处理、基准→结构树筛选/基准；专用素材到位后改路径即可）。
  - 回归：workspace `tools/r37test` 42 断言 ALL PASS（四占位页行集/控件值/
    全禁用/零提交探针=撤销栈双零；曲线样式页列表回显/勾选提交 undo/批量
    宏/图例显隐/外观行落当前曲线/plot3dType 启停/图表删除兜底；同页重复
    挂载实例复用；E 窄宽防线五页 142px）。开发为派发 subagent 配额截断前
    完成（965df64e），项目经理验收补测：r37test 两处**测试脚本缺陷**修复
    （C5/C6 撤销栈基线 count→index——undo 残留 redo 分支被 push 截断的
    QUndoStack 标准语义；D 段误将栈上页面 setParent 局部宿主=父析构删页
    后函数尾栈析构双重释放，改对齐真实换页语义 hide+setParent(nullptr)），
    产品代码零改动；既有回归重建后全过：3dtest（tst_3dchain/tst_curve3dpage
    42/tst_r33qa 15/tst_qwa 9 场景/tst_r36 48）+axis3dtest 75+r34test 83+
    ctest 基线一致（PostWindowTests/PostWindowIsolation 两失败系基线遗留）。

- **色带类三维图"外观与着色"行集按类型区分 + 曲面显示模式（R38，20261113）**：
  三张节点属性页设计稿提供于 design/3d-curve/（不入 git；ref-3dcurve-bar/waterfall/
  surface-prop-page.png，用户 20261113 拍板三处口径=①仅曲面显示模式接引擎
  已有能力、其余置灰占位 ②柱状与瀑布/曲面同行集（柱状截图在颜色行下裁切）
  ③R37 曲线样式工具页本轮不动；**同日二次拍板勘正②：柱状外观组只有颜色
  一行**——柱状截图本就完整，初版误按四行实现已修正）。mCurve3DPropPage 同
  一"外观与着色"折叠组
  内**两套行集按 plot3dType 二分**（FoldSection setRowVisible 行摘除/插回
  机制：行序按 addRow 逻辑序插回、内容区高度同步自带）：曲线类（==1）九行
  原样（R36 口径零回归）；色带类行集=曲面/瀑布（0/2）**四行**、柱状（3）
  **仅颜色一行**（勘正口径）=显示模式
  [QComboBox 三选项 着色曲面/等高线/着色曲面+等高线（下标=kDrawStyle 值）——
  **仅 plot3dType==2 生效**（connect 带类型守卫提交，占位类型程序化触发零
  commit），瀑布置灰固定单选项"着色曲面"（引擎无样式 API）；柱状无此行]/
  颜色
  [mCurveColorButton 通栏色条（R34 B02 bar 形态），置灰静态色照截图取色备案
  （瀑布=蓝 85,116,189/曲面=橙 221,147,96/柱状=紫 145,112,188），同一控件
  三类型显示不同静态色=loadFromModel 按类型 setColor（阻塞信号；置灰态 Qt
  照常显色）]/不透明度[置灰静态滑条（柱状无此行），滑位照截图就近取整备案：
  瀑布 65/曲面 50]/云图[[勾选框+变量下拉+"编辑图例..."钮]三控件容器全置灰
  占位（柱状无此行；钮 tooltip 同 R36"色带图例编辑下一期"）]。占位纪律
  R31/R33 同款逐条
  （构造即禁用、不连 commit/信号、不进回填）；两套行集可见性/启停切换集中
  updateAppearanceRows 单一函数（loadFromModel 末尾调用，仿
  updateStyleRowStates 模式）。**模型**：CurveSeriesData.drawStyle（int 默认
  0，语义=引擎 DrawStyle 同值）+CurveProp.kDrawStyle；会话快照 **V4→V5**
  （Curve 节点追写 drawStyle，读侧版本门控+越界钳 0..2，V4 及更旧快照缺省 0
  零行为变化；模板路径 writeTemplateNode/readTemplateNode 不动零波及）；
  nodeProperty/setNodeProperty 两处路由（同值短路等既有语义不变）。**渲染**
  （m3DChartView）：rebuild 建曲面序列（plot3dType==2）分支 setData 之下发
  `setDrawStyle(qBound(0, drawStyle, 2))`——**顺序纪律=setData 先行**（等
  高线 VBO 在 setData 的 rebuildContours 已建，样式切换仅绘制期分支消费；
  缺省 0=引擎默认 StyleSurface 与旧渲染逐位一致零回归）；曲线id→
  shared_ptr<Surface3DSeries> 映射（仿 R33 _lineSeries，rebuild 开头随
  _lineSeries 一并 clear，shared_ptr 防悬挂）；样式增量分流纳入 kDrawStyle
  （判定序仍在 legendPropRelevant 之后/rebuildRelevant 之前，命中即 return；
  映射无该 id=隐藏曲面/非曲面序列安全短路；不可见期不碰画布改
  scheduleRebuild 挂起、showEvent 补跑——B03 口径；rebuildRelevant 白名单
  **不追加** kDrawStyle 防双触发，R33 同款；引擎 setDrawStyle 自带 update，
  宿主不手动刷画布）。**引擎（Plot3D/PlotView）零改动**
  （Surface3DSeries::setDrawStyle 既有，写成员+m_viewer->update()）。
  回归：workspace `tools/3dtest/tst_r38` 新增 50 断言（行集二分曲线类/三类
  色带/同实例双向切换/占位静态值=截图备案/窄宽防线/曲面显示模式提交栈增量
  恰 1+undo/redo 回显/占位程序化撤销栈双零/增量画布实例+相机矩阵不变/
  表面→等高线→复合像素形态断言（非白像素 41777→13801、undo 逐档逐像素
  复原）/隐藏期挂起重显补跑/V5 往返+快照版本号+越界钳制+V4 手构字节块
  兼容）；既有套件**仅定位适配**（页面新增 6 个置灰占位控件：tst_curve3dpage
  S3c 占位普查 7→13、tst_r36 云图勾选框/变量下拉定位改构建序首个——断言语
  义零改动）；全套重建后过：3dtest（tst_3dchain/tst_curve3dpage 42/
  tst_r33qa 15/tst_qwa 9 场景/tst_r36 48）+axis3dtest 75+r34test 83+
  r37test 42+r37qa 12+ctest 基线一致（PostWindowTests/PostWindowIsolation
  两失败系基线遗留）。**柱状仅颜色一行勘正（同日二次拍板，B01 口径）**：
  updateAppearanceRows 色带类可见性改为 曲面/瀑布四行、柱状仅颜色
  （bandWithModes=plot3dType∈{0,2}）；tst_r38 S3d/S3f 与 tst_r38qa A3 断言
  同步改为柱状显示模式/不透明度行不可见+折叠组仅颜色一行，tst_r38 50+
  tst_r38qa 18 重跑 ALL PASS。

- **三维图注释画布渲染与画布管理二期（R42，20261113——T42-1；R34 一期
  "界面+模型接线"的画布侧收官）**：注释显示在 3D 画布+画布交互+四类翻活，
  范围经用户 20261113 拍板。**模型两新键**：`CurveAnnotationData.borderShow`
  （显示边框，缺省 true）与 `CurveChartData.annotationsVisible`（注释图表级
  总开关，缺省 true，对 2D 图表无消费零影响）；CurveProp 键表 kBorderShow
  （仅 Annotation 节点放行）/kAnnotationsVisible（仅 Chart 节点放行）经
  nodeProperty/setNodeProperty 两处分发（键→结构字段映射仿 kAlign/kBorderWidth
  口径）。**会话快照 V5→V6**：写侧在既有 annotation/chart 字段落串之后**追写**
  quint8 bool（Chart 先 annotationsVisible、Annotation 后 borderShow，写侧恒写
  占位保持格式一致），读侧 `snapshotVersion>=6` 门控读入——V5 及更早 .mxses
  零损伤（两新键缺省 true）；**模板路径 writeTemplateNode/readTemplateNode 零
  改动**（Annotation 整结构体共享 operator<< 同时被模板路径复用、V2 字段序
  冻结不得追加字段——否则模板读入错位；模板不含新字段→恢复缺省 true）。三
  落点：
  - **控制器**：共享创建例程 `addWindowAnnotation(chartId)`（宏"新建注释"内
    addAnnotation 默认计数名+kAttachMode=1+kText="新注释"+kPosX/kPosY=50，
    一次 Ctrl+Z 整体撤销含文本/位置初值）——**kPosX/kPosY 在 3D 窗口注释语境
    =画布百分比 0-100、框左上角锚点**（字段同名复用注释结构体，2D 数据坐标
    语义不变，3D 渲染/拖拽统一按百分比口径）；Ribbon"注释"按钮与页面"新增"
    两入口同源（mPostMenuContributor 本地宏拼装删除，启停联动/防御守卫零
    变化）。**注释剪贴板**（控制器成员值快照，会话级内存不序列化，
    clearDocument 一并失效；与节点剪贴板 copyNode 族相互独立）：
    copyAnnotation（深拷贝 name 之外全部 CurveAnnotationData 含显隐态）/
    cutAnnotation（copy+删除命令包宏"剪切注释"，一次撤销恢复原节点、剪贴板
    不受撤销影响）/pasteAnnotation（宏"粘贴注释"内 addAnnotation→逐字段提交
    kText/kPosX/kPosY/kAlign/kColor/kFont/kBgColor/kBorderWidth/kBorderShow/
    kAttachMode/kVisible——全字段一致含显隐态，剪切→粘贴恢复语义；位置=原值
    原样不偏移）+hasAnnotationClipboard（粘贴钮可用态判定）。
  - **画布渲染与交互**（`CurveView/m3DChartAnnotationItem` 新增+宿主
    `m3DChartView` 集成）：每个可见注释一个 QWidget 子控件（宿主视图子控件
    非画布子控件——R32 图例覆盖层先例，rebuild 整画布换代不受影响；集合级
    重建=旧项 hide+deleteLater+列表立即换代防悬挂），rebuild 尾按文档树序
    重建全部注释项（可见=图表 annotationsVisible 且注释 kVisible；关闭/无注释
    →零项不崩溃）并 raise 在图例覆盖层之上。paintEvent 自绘：背景（kBgColor
    无效→半透明白 QColor(255,255,255,200) 图例同款）/边框（kBorderShow 且
    kBorderWidth>0 画框；borderMode==1 且 kBorderColor 有效→自定义色否则默认
    深灰）/多行文本（kFont+kColor 无效→#333 同页口径；kAlign 0左/1中/2右 逐行
    水平对齐；固定内边距 4px）；定位=画布百分比框左上锚点（图例 position==4
    同语义）、按内容现算自适应宽高、宿主 resizeEvent 百分比重摆越界收敛。
    **模板求值渲染期现算**（kText 恒存模板原文）：{Label}→节点本名；
    **{X}/{Y}/{Z}→光标实时底面数据坐标**（`QString::number(v,'g',6)`；光标
    离画布保持最后值；从未悬停显示"—"）；{U}/{V}/{W} 及未知 token 字面保留
    （含花括号原样）。**XYZ 反投影**（_plot setMouseTracking(true) Qt 公开
    API+eventFilter 增捕 MouseMove；**无按键 move 在过滤器内消费不上送引擎**
    ——引擎 InputController 对无按键 move 并非空操作（图例悬停判定+
    requestPicking 离屏 ID 渲染拾取），R42 前跟踪关闭该路径不可达，消费后引擎
    收到的 move 事件集与 R42 前逐位一致、交互语义零变化；带按键 move 照常
    上送=旋转/缩放/色带拖动不受影响）：鼠标 NDC
    （2x/w-1, 1-2y/h）→inverse(proj*view) 近/远两点成世界射线→与世界平面
    Z=-10 求交（**BOX_SIZE=10 归一化约定**：数据→(v-min)/range*20-10∈
    [-10,10]³，Z 最小数据面=世界 Z=-10；正交默认投影直线求交、透视按射线
    方向参数化同式）→(x,y)∈[-10,10] 经 normalizedBoxToData 反算数据坐标
    （range 与 queryAxis3DDisplayRange 同源=渲染归一化同一 true 源，rebuild 尾
    缓存，空图无效→"—"）；{Z}=底面数据坐标=Z 显示范围最小值。反投影/换算/
    模板求值以静态 seam 公开（groundWorldFromNdc/normalizedBoxToData/
    evaluateTemplate）供 annotest 断言（算法可追溯）。**交互**（项仅覆盖自身
    矩形，框外事件天然到 GL 画布，相机旋转/缩放零影响）：左键按下记录起点→
    移动超 ~4px 判拖拽=项内实时重摆（视觉跟随不逐帧提交）→松开宏"移动注释"
    提交 kPosX+kPosY 一次（按松开时百分比反算；提交信号回落 refreshFromModel
    按已提交百分比重摆=原位无跳变）；按下到松开未超阈值=点击选中
    setCurrentNodeId（树回选+参数栏换绑既有全链）。**刷新分流**（四分支互斥，
    判定序在 legendPropRelevant/styleRefreshRelevant 之后、rebuildRelevant
    之前）：`annotationPropRelevant`=本图表 Annotation 子节点×{kText,kAlign,
    kColor,kFont,kBgColor,kBorderWidth,kBorderShow,kVisible,kPosX,kPosY,kName}
    ∪ 本图表 Chart 节点×kAnnotationsVisible → 单项 refreshFromModel/集合级
    重建注释项，**禁止整场景重建**（相机不丢）；rebuildRelevant 白名单不追加
    这些键防双触发（R33 教训；注释节点 kVisible 先被本分支截获，曲线 kVisible
    仍走重建）；注释增删走既有结构信号（R35 广谱删除+nodeAdded 归属过滤）→
    scheduleRebuild→rebuild 尾重放，撤销/恢复正确；不可见期挂起 B03 口径
    （scheduleRebuild 置 _rebuildPending、showEvent 补跑重放）。删除清理=
    集合级换代+项随宿主视图析构双防线（无悬挂）。
  - **页面翻活**（mAnnotation3DPropPage）：显示注释（回显=父图表
    kAnnotationsVisible、勾选=setNodeProperty(chartId,kAnnotationsVisible)）与
    显示边框（回显/提交 kBorderShow）两行翻活（回显全程 QSignalBlocker）；
    {X}{Y}{Z}{Label} 四模板钮翻活（点击=向文本编辑区光标处 insertPlainText
    插 token 文本，不直接 commit 维持失焦提交口径；{U}{V}{W} 维持置灰占位）；
    剪切/复制/粘贴翻活（控制器剪贴板 API，粘贴后 setCurrentNodeId 到新注释；
    钮可用态 copy/cut=绑定注释有效、paste=剪贴板非空，updateButtonStates 于
    回填尾与操作后刷新）；addNewAnnotation 改调共享例程（两入口同源）。
  - 回归：workspace `tools/annotest` 新增 51 断言 ALL PASS（创建默认值+一次
    撤销含初值/非法 chartId；画布项 无注释零项/50% 锚点定位/resize 百分比
    重摆/九键提交 GL 画布实例+相机矩阵不变（R36 同款断言）/kVisible 单项显隐/
    kAnnotationsVisible 全显隐/拖拽宏一条+undo 恢复/点击选中栈零增量/增删随
    结构信号重放；序列化 V6 版本号+两新字段双态往返+V5 手构字节块容错缺省
    true+模板 V2 字段序冻结间接证；剪贴板 copy→paste 全字段一致+默认计数名/
    cut→删除→粘贴恢复/三操作 undo·redo 逐档/空剪贴板 -1/非注释 copy 拒绝不
    污染；求值 seam normalizedBoxToData 三点/正交中心角点/透视中心/
    {Label}=本名/未悬停"—"/悬停实时值/{U}{V}{W} 未知 token 字面/
    needsCursorTokens 过滤；页面 翻活行回显提交 undo/占位普查恰 {U}{V}{W}+粘贴
    空态/新增走共享例程/模板钮插 token 未失焦零提交失焦一次/三钮可用态）；
    `r34test` 占位断言更新为生效口径后 84 断言 ALL PASS（B02 行级最小宽断言
    保持，11 枚行按钮机制不变）；3dtest 五套+axis3dtest+define3dtest 全过；
    ctest 基线一致（PostWindowTests/PostWindowIsolation 两失败系基线遗留）。
    测试抓出并修复产品缺陷一处：buildUi 漏接显示注释 toggled→
    kAnnotationsVisible 提交（annotest F4 首跑暴露，已补 connect）。
  - **同日用户二次拍板修订（推翻首轮"屏幕空间覆盖层+光标底面坐标"两口径）**：
    ①**注释锚定三维数据空间**——注释框锚点=posX/posY/posZ 三维数据坐标
    （kPosZ 新键，复用基准面侧 R41 同名键、分发按节点类型区分；会话快照
    **V6→V8**（中间 V7 系 R41 基准面占用），Annotation V8 段追写 posZ double，
    读侧 >=8 门控缺省 0，V7 及更早零损伤；模板路径零改动），渲染=宿主
    projectDataToPixel 按相机 proj*view 投影到画布像素定位（框左上角=锚点投
    影位、越界收敛钳回画布、锚点在相机身后 w≤0 项收场隐藏），画布每帧 Paint
    后统一重投影（视角旋转/缩放/平移像素位随动）；创建默认锚点=轴显示范围
    中点（addWindowAnnotation 改签名入参三键，调用方经 queryAxis3D DisplayRange
    现算，无可见数据退化 0,0,0）。②**{X}{Y}{Z}=注释自身位置**（用户原话
    "x,y,z 的值应该是注释的位置而非鼠标"）——首轮光标反投影机制整体拆除
    （groundWorldFromNdc/setMouseTracking/MouseMove 过滤器消费全撤，引擎交互
    语义恢复 R42 前原状），evaluateTemplate 直接现读锚点三键；拖拽=松开时
    unprojectPixelThroughData 把框左上角像素反投影到"过原锚点、视向为法向"的
    相机平行面（深度保持）得三键提交一条宏。附着方式行显示文案"窗口"→"三维"
    （kAttachMode=1 模型值不变）。annotest 更新为 50 断言（投影往返/锚点三键
    往返/V5+V7 手构容错/页面中点默认值）+r34test 附着方式文案断言更新，全套
    件与 ctest 基线复跑全过（3dtest/r36 首跑失败系套件二进制早于 posZ 结构体
    变更的 ABI 陈旧，全量重建后复归全绿）。
  - **实机缺陷修复（用户实测反馈"拖拽后闪回"+"点击注释未直接创建"，探针
    workspace tools/annotest/tst_probe_anno 定向实证）**：三根因三修复——
    ①**annotationPropRelevant 键表白名单漏 kPosZ**（主根因）：拖拽提交三键
    时 kPosX/kPosY 各刷新一次（旧 z 投影）、kPosZ 写入后无任何刷新，项恒停
    "新 x 新 y 旧 z"投影位=每次拖拽必然错位闪回；kPosZ 入白名单一行修复。
    ②**反投影 toVector3DAffine 除零毒化**：像素差分方向向量 w=0，toVector3D
    Affine 按 affine 语义除 w → inf/NaN → 提交值塌回原锚点 → 同值守卫跳过
    提交（探针实证提交值与上一次完全相同）；改显式 QVector3D(x,y,z) 构造。
    ③**reposition 拖拽态守卫**：拖拽中任何 GL 重绘的排队重投影不得移动拖拽
    中的项（防御性，与②共同闭合闪回）。辅助改进：**新建锚点级联偏移**——多
    注释精确堆叠同一锚点，拖走其一其余原位=观感"未新建/闪回"，按现存注释数
    X/Y 各偏移 3% 量程（%8 循环），页面与 Ribbon 两入口同口径；"点击注释只
    弹参数栏未创建"经探针 R1-R3 证伪（Ribbon 流节点入库+画布项出现+画布内
    可见均通过），实机观感系①③闪回+堆叠的复合误判。回归：探针 7 断言
    ALL PASS（8 次拖拽逐帧稳定/提交落模型/中途重绘不抢位/提交后恒等闭合）+
    annotest 50+r34test 84+3d 全套+axis75+define 四套 380 项+ctest 基线一致。

- **三维图注释对齐二维——Ribbon 检出模式+内容打点创建牵引线注释+参数栏注释
  管理组退役（R46，20261117；用户需求："点击注释功能后处于激活状态，在曲线/
  曲面/柱状等内容中点击像二维那样牵引线+注释，外部点击保持现状；参数栏去掉
  注释管理"）**：
  - **视口工厂三维注释模式**（mCurveViewportFactory，Ribbon 模式全局开关同
    二维注释模式架构）：`setAnnotation3DMode(bool)`+`sig_annotation3DMode
    Changed(bool)`+`_annotation3DMode` 成员。前条件=存在已登记的三维图卡片
    （chartType==5，含未物化——按钮启停联动已保证激活图表为三维图才可点，
    此处防御兜底；被拒警告"注释：请先打开三维图视窗"+广播 off 回弹按钮）；
    与二维注释模式互斥（两按钮同为"注释"概念，一次只允许一种激活——双向
    互踢：开启任一方先关另一方；二维侧既有互斥链[基准线/编辑曲线/定义曲线]
    随其关闭自然联动）；新物化三维图视图经 materializeChart 承接当前模式
    （原"三维图无注释语义跳过"备案随本轮失效）；最后一个视口/卡片关闭自动
    退出（handleChartViewRemoved/closeChartViewport 与注释/基准线同口收口）。
  - **画布打点创建**（m3DChartView）：`setAnnotation3DMode` 开关状态+画布十
    字光标（rebuild 重建画布尾按标志重设）；**左键截获**在既有 `_plot` 事件
    过滤器（Paint 分流旁新增鼠标分流）——按下/松开均消费不进引擎输入控制器
    （相机不再进入旋转态，右键/中键/滚轮不截获=模式内仍可平移缩放导航）；
    按下到松开 ≤4px 判单击创建（拖拽不创建防误打点，左键按下已被消费拖拽
    无相机语义）；**宿主侧内容拾取** `pickDataPoint`：可见曲线逐采样点投影
    （proj*view 现读一次+逐点直乘，刻度轴逐点变换、变换 NaN 点不入拾取），
    取点击位最近采样点（半径 30px 与二维 snapper kAnnotationSnapPx 同值），
    命中回传原始数据坐标——引擎选区缓冲拾取为引擎私有路径不可达（R42 拆除
    MouseMove 跟踪后 requestPicking 无触发源）且需改引擎公开 API，宿主投影
    拾取与二维"最近采样点吸附"语义对齐且零引擎改动；**命中→宏"添加注释"**
    `createAnnotationAt`：addAnnotation（默认计数名）+kAttachMode=0（3D 语义
    =内容锚定带牵引线，复用 2D"曲线类=0"档位值）+kText=模板原文（沿用本图表
    最近含 '{' 的注释模板——3D 判据=含 token，旧默认"新注释"纯标签不作模板
    传播；无则默认 "X={X}\nY={Y}\nZ={Z}"，渲染期按锚点现算，对齐 2D"模型存
    模板"纪律）+锚点三键=拾取点；创建后 setCurrentNodeId（树回选+参数栏挂页）。
    **未命中=内容外点击不创建且模式保持现状**（用户口径"外部点击保持现在的
    状态"，模式内可继续打点/换视角后再打点）。**注释增删不再整场景重建**
    （R46 分流）：sig_nodeAdded 命中"本图表 Annotation 节点（可查类型+归属
    过滤）且可见期"→ 仅 rebuildAnnotationItems（相机/GL 场景不动——连续打
    点不丢视角，此前 R42 口径=结构信号广谱 scheduleRebuild 每次创建整画布
    换代重置相机，模式打点工作流不可接受）；sig_nodeRemoved 删除侧节点类型
    不可查，按 `_annoItems` 集合成员反推（id 在集合=可见注释删除）同款仅重
    建注释项，集合外（隐藏注释/他图表）走既有广谱分支。
  - **牵引线注释项**（m3DChartAnnotationItem）：渲染按 kAttachMode 两形态
    分流——0 内容锚定=牵引线+注释框（框左上角=锚点像素+固定偏移 +35,-32，
    对齐 PlotView AnnotationSettings::curveBoxOff 2D 曲线类同款；本项几何=
    框∪锚点包围盒含 2px 笔画余量；牵引线=锚点→框中心方向止于入框处框边、
    边框有效色 1.2px 局部抗锯齿——PlotView 引出线同款画法，锚点在框内不画；
    框/锚点均越界收敛钳回画布）/1 窗口=既有口径零变化（框左上角=锚点像素，
    存量会话/剪贴板粘贴不受影响）；拖拽语义扩展：松开反投影取**锚点像素**
    （形态 0=项位置+项内锚点偏移，形态 1=框左上角即锚点，既有口径不变），
    整体移动（框+牵引线+锚点随行）提交三键口径不变；点击选中/拖拽阈值/
    宏"移动注释"全链不变。
  - **Ribbon 入口改模式开关**（mPostMenuContributor 3D曲线-标注-注释钮）：
    setCheckable(true)，点击=工厂 setAnnotation3DMode(checked)，按下态经
    sig_annotation3DModeChanged 回弹（开启被拒/互斥踢出/末视口关闭自动退出
    均随信号）；启停联动（chartType==5 才启用）零改动；旧"点击直接创建中点
    注释"入口退役（级联偏移/queryAxis3DDisplayRange 默认锚点随入口退役，
    m3DChartView.h include 移除）。控制器共享例程 addWindowAnnotation 与注
    释剪贴板 API（copyAnnotation/cutAnnotation/pasteAnnotation）保留不删
    （剪贴板/存量会话路径可达，模型零改动零序列化迁移）。
  - **参数栏注释管理组退役**（mAnnotation3DPropPage）：注释管理组（显示注释/
    注释名下拉/注释列表/新增·剪切·复制·粘贴）整组移除——注释新增=画布打点、
    单项显隐=树勾选（kVisible 既有）、图表级总开关=树/后续入口、注释间导航=
    树选中/画布点击注释项（setCurrentNodeId 全链既有）；构造器兄弟节点全局
    监听（仅为下拉/列表内容刷新而设）一并移除，页面聚焦单注释编辑：文字/
    属性/附着对象三组 9 行不变（回显纪律/失焦提交/占位普查口径不变，置灰
    占位余 {U}{V}{W} 三钮+附着方式）；附着方式行文案仍回显"三维"——R46 起
    模型档位 0=内容锚定/1=窗口两态并存（均锚定三维，不提供交互切换）。
  - 回归面：annotest/r34test（用户 workspace 工具件，未随本仓检出）覆盖的
    页面行集/占位普查/新增例程断言需随本轮同步适配（注释管理组行集退役）；
    引擎（Plot3D/PlotView）零改动，模型键/序列化版本零改动（kAttachMode 复
    用既有字段）。
  - **同日用户实测四问题修订（20261117）**：
    ①**内容外点击无注释/参数栏常不跳出**——两问题同根：首版"外部点击不创建"
    口径与用户预期不符（对齐二维应产生视图类注释，创建即选中=参数栏跳出），
    加之拾取只测采样点顶点，稀疏曲线/粗网格曲面"段中/面中"点击漏检（"很多
    时候"）。修订：`createAnnotationAt` 未命中改走 **`createWindowAnnotationAt`**
    ——像素反投影到"过显示范围中心、视向为法向"相机平行面（差分 through=
    变换空间中点，与拖拽反投影同一实现）得三维锚点，宏"添加注释"内
    addAnnotation+kAttachMode=1（窗口=框左上角即锚点无牵引线，对齐二维视图
    类档位值）+kText="新注释"+锚点三键，创建后 setCurrentNodeId（两类点击均
    产生注释且模式保持现状）；`pickDataPoint` 加密候选点——曲线/瀑布加相邻
    采样对中点、曲面（rows×cols 有效栅格）加单元四角平均中心点（候选点=原
    始坐标插值/平均，testPoint 单实现三类共用），对齐二维吸附"曲线上任意位
    置"语义。
    ②**拖拽应为"牵引线拉长、{X}{Y}{Z} 不变"（对齐二维拖框语义）**——首版拖
    曳=锚点三键随动（框+线+锚点整体平移），{X}{Y}{Z} 跟着变。修订：**模型新
    增框位置三键** `boxPosX/boxPosY/boxPosZ`（CurveAnnotationData 三 double
    字段，缺省 qQNaN()=未手动摆位框=锚点+固定偏移；CurveProp 新键
    kBoxPosX/kBoxPosY/kBoxPosZ 仅 Annotation 节点放行；共享 operator<< 字段
    序冻结不追加，会话快照 **V13→V14** Annotation 段在 V8 posZ 之后恒追写三
    double、读侧 ≥14 门控缺省 NaN，旧 .mxses 零损伤；模板路径不含新字段）。
    注释项 refreshFromModel 消费三键（有效=框左上角按框位投影摆框、牵引线随
    框位拉长/转向；NaN=锚点+偏移；框位投影失败回落偏移），拖拽松开分形态提
    交——内容锚定=框左上角像素反投影（through=已摆位框位自身/首拖回落锚点
    深度平面）提交框位三键锚点不动；窗口=框左上角即锚点提交锚点三键（既有
    口径不变）；`annotationPropRelevant` 白名单追加三键（缺键=拖框后不刷新，
    R42 闪回同口径）。
    ③**吸附高亮（用户问题4）**：模式内无按键 hover move 在事件过滤器消费不
    上送引擎（引擎 InputController 对无按键 move 非空操作，R42 读数拆除先例
    同款=引擎收到的 move 事件集与模式开启前逐位一致；带按键 move 照常上送），
    hover 拾取命中显示**蓝色空心圆环+中心点吸附标记**（SnapMarkerOverlay，
    视图子控件+鼠标穿透+懒创建，QColor(43,125,233) 对齐二维悬停预览色系），
    未命中/Leave/退出模式收场隐藏；命中点数据坐标现存（`_snapValid/_snapPoint`），
    相机 Paint 同步链随帧重投影（视角旋转/缩放随动）；画布 `setMouseTracking(true)`
    开启 hover 投递（rebuild 尾重设）。创建后按松开点重拾吸附态（光标处可立
    即续打点）。
    ④回归面新增：`r34test`/`annotest` 附着方式文案/行集断言同①适配；序列化
    断言需补 V14 往返+V13 手构容错缺省 NaN。
    ⑤**拖拽实时反馈+选中高亮（同日二轮，用户反馈"移动时牵引线以及位置信息
    实现刷新/选中时高亮提示"）**：
    - **内容锚定形态拖拽=锚点像素恒定、仅框实时平移**（mouseMoveEvent 重写，
      拖拽进入即记录三基准[项左上角/锚点像素/框左上角，父系坐标]）——牵引线
      逐 move 重算重绘（几何=框∪锚点包围盒现算+update()），实时拉长/转向，
      {X}{Y}{Z} 天然稳定；松开提交框位三键口径不变（消除首版"整项平移松开
      后锚点跳回"的两段式观感）。delta 数学：mapToParent(p-press)=M-press
      （M=鼠标父系坐标），框位=起始框位+(M-M0) 恒等闭合。
    - **窗口形态拖拽=整项平移（钳回画布）+位置信息实时刷新**：拖动中锚点像
      素=项左上角，反投影（过模型锚点相机平行面）得临时数据坐标，模板按临时
      坐标现算文本（evaluateTemplate 新增指定坐标重载；不提交，松开提交后
      refreshFromModel 按模型值重算=无跳变）；反投影失败保持现文本。
    - **选中态=原边框加粗+变色**（同日二轮用户拍板，替换首版"框外另描细线"
      方案）：选中时本框边框改主题蓝 QColor(43,125,233) 并加粗一档（宽度=常
      态有效线宽+1px；显示边框关断/零宽按 1px 基准加粗到 2px，选中提示恒可
      见），取消选中回落既有边框口径。项构造连选择中枢 sig_currentNodeChanged
      （两态翻转才 update）+refreshFromModel 按中枢现值回填（重建项不丢选中
      态）。**拖拽完成保持选中**（同日四轮用户拍板，推翻二轮"移动完成即清除"
      ——拖完高亮随选中保持，取消只经下方两条清除路径）。
      **同日三轮交互规则补齐（用户反馈"未选中也能拖/点别处选中不消失"）**：
      ①**未选中不可拖**——mouseMoveEvent 对非选中项静默忽略拖拽（按下仍被项
      消费=画布不在其下转相机/建注释；松开 ≤4px 仍走点击选中、>4px 不选不
      拖），工作流=先点选（加粗蓝边）再拖动，拖完选中保持；②**点击画布空白
      处清除注释选中**——宿主 eventFilter 新增画布左键按下分流（先于注释模
      式分支，模式内点空白=清除旧选中→创建新注释并选中，终态正确）：命中注
      释项框体的按下被项自身消费不达画布=不清（点另一注释→其自身点击选中顶
      掉旧选中）；未命中任何注释项时当前选中若为注释节点则写空中枢。不消费
      事件=相机交互零影响；仅清注释节点，不误伤图表/曲线选中。

- **三维基准面鼠标拖动——画布直接拖拽移动+基准面属性保视角增量分流
  （R48，20261117；需求"创建基准面后应该可以使用鼠标进行拖动基准面"）**：
  R41 基准面此前仅参数栏页数值定位（且属性行提交走全量 rebuild=rebuild 尾
  fitToView 重置相机），本轮接通常态画布交互。两落点（引擎零改动）：
  - **拖拽交互**（m3DChartView eventFilter 常态[注释模式外]新分支）：左键
    按下先宿主解析拾取基准面（`hitTestDatumPlane`——引擎 DatumPlane3D 非
    拾取图元 getDataCount=0，按法向轴位置四边形[盒空间另两轴满幅
    [-10,10]、位置钳盒=与引擎 draw 重建同位]经 proj*view 投影成画布凸四边
    形做点包含测试，树序迭代取最后命中=后绘制在上；色带图例三矩形内让路
    引擎图例拖动；图表级总开关关/单基准隐藏/位置变换 NaN[log/dB 轴 ≤0
    未建面]不参与），命中即截获消费（引擎相机不进入旋转态）+SizeAll 光标；
    拖动 `datumDragMove`=像素差分反投影（过**模型提交位**相机平行面——拖
    拽期文档不动=固定差分基准，多次 move 绝对映射自洽，unprojectPixelThroughData
    注释拖拽同一 seam）取**法向轴分量**（平面只沿法向移动，另两轴分量丢弃；
    正交投影下相机平行面含法向轴=法向指向相机也无交互死区）、钳显示范围
    换回渲染空间值引擎 setPosition 实时预览（成员写+标脏+update，文档不
    动相机不动）；松开 `datumDragCommit`=预览位（图元 position 现读）逆变换
    回原始数据值与模型现值比对，无变化不发（原地点击/拖回原位不压空操作
    宏），有变化=宏"移动基准面"单键提交法向轴对应坐标键（一次 Ctrl+Z 整体
    撤销；提交信号经增量分流回写同值图元=视觉零跳变，基准页属性行随文档
    信号回显）。悬停反馈=命中基准面显 SizeAll 并消费 move（引擎 cursorChanged
    对无按键 move 恒发 Arrow 会覆盖自设光标；基准面非引擎拾取图元跳过
    requestPicking 无功能损失），未命中放行（引擎箭头/图例光标+离屏拾取
    语义零变化）；捕获中左键丢失（move 无左键态）按提交收场兜底；拖拽中
    场景换代/基准删除（映射换代 constFind 落空）安全终止捕获；注释模式开启
    跨态兜底弃用捕获+预览位回弹模型值。注释项框体自身点击被项消费不达画
    布=框体交互不受影响。
  - **基准面属性保视角增量分流**（datumPropRelevant/applyDatumIncremental，
    sig_nodePropertyChanged 处理链注释分流之后新增，R33/R38 先例）：本图表
    DatumPlane3D × {kPosX,kPosY,kPosZ,kNormalAxis,kColor,kDatumOpacity,
    kText,kLabelPos,kName} 九键命中=按键映射引擎 setter（位置族=法向轴坐标
    qIsFinite 防御+轴刻度变换 setPosition，与 rebuild 基准段同一取值口径；
    kName/kText 合流标签=text 空取本名；透明度非有限回退 0.25）+update()，
    可见期禁止整场景重建——**拖拽提交/属性页改基准位置不再重置相机**
    （原 R41 口径全量 rebuild 的 fitToView 相机跳变就此消除）；不可见期
    scheduleRebuild 挂起（B03 口径）。rebuildRelevant 基准键表同步剪除九键
    （防双触发 R33 教训），保留 kVisible（翻可见建图元/翻隐藏撤图元走全量
    重建）与 kDatumType/kReverseClip（零渲染消费字段，与 R41 一致）。
  - 回归：本机增量编译+链接通过（PostCurve Debug，仅既有 STL 弃用警告）。
    仓内测试（tst_mDatumPlaneModel 模型级/tst_m3DPlotDataParser）不触视图
    重建路径零影响；workspace annotest/3dtest 套件不在本机，待宿主测试
    工作区复跑（关注点：基准键提交由 rebuild 改增量分流后的像素断言面——
    观感应逐位一致[引擎 setter 与 rebuild 下发同值]，仅相机不再重置）。


- **三维数值页寻点接入——曲线下拉+极值导航+画布寻点标记+坐标数据表高亮
  （R47，20261117——T47-1；R37 数值占位页翻活，需求
  20261117-三维数值页寻点与坐标数据表接入.md，用户三项拍板[完整语义/表格
  四钮保持置灰/在途改动叠加交付]）**：R37 纯置灰占位的 mPointSeek3DPage
  接入为可用寻点工具。四落点：
  - **寻点核心**（CurveCore/mPointSeekCore.h/.cpp 新建，纯静态工具类无 GUI
    依赖可离屏单测，mCurveExpressionEvaluator 同款）：输入一条三维曲线点集
    （x/y/z 三列等长 QVector<double>，pointCount 按最短列防御），输出——
    列语义 0 极值（作用坐标 z，步进序=下标序）/1 X轴（x 升序**稳定**排序
    std::stable_sort 等值保原下标序）/2 Y轴（同口径作用于 y）；isValidPoint
    =下标有效且三坐标均 qIsFinite（NaN/±inf 断口点不可寻/不可停驻/不判峰谷
    ——修订四非有限值口径）；globalMax/MinIndex=该坐标有效点极值（并列取
    下标最小者）；localPeak/ValleyIndices=沿下标序扫描、峰=该坐标严格大于
    两邻（端点单侧判定、邻点该坐标非 finite 按该侧不存在、平台值不判峰——
    严格大于口径）、谷=严格小于镜像；stepSeek=步进序上前后一步**止于端点
    不环绕**（fromIndex 不在序内=dir>0 落序首/dir<0 落序尾）；nearestExtremum
    =沿下标序方向最近峰/谷（无峰谷返回 -1 原地不动不环绕）；
    coordinateVaries=点数<2 或该坐标全部 finite 值极差==0（全程恒值）→整列
    禁用。精度约定=double 域直接比较（恒值判定极差恰 0 无需容差），头注释
    注明对齐 HyperGraph 寻点行为。
  - **页面接线**（mPointSeek3DPage 重写，布局骨架保留——mCurveFoldSection
    两节[寻点与坐标/坐标数据表]+按钮网格 16 枚导航小钮）：曲线下拉=当前
    激活三维图（工厂 currentChartId+sig_currentChartChanged，mDatum3DPage
    同款"页面自刷新"挂载链零改动）全部 Curve 子节点树序（四类 plot3dType
    均列；R23 重名允许 itemData=节点 id 键定，刷新保选择按 id 重定位落空
    自动选首条）；文档信号 sig_nodePropertyChanged/nodeAdded/nodeRemoved
    全局监听（mCurveStyle3DPage 先例）整体刷新，"data" 数据直写信号触发
    当前点有效性复查（越界/该点非 finite→标记与高亮清除）；坐标数据表=
    选中曲线全部点四列只读（NoEditTriggers，序号 1..N+'f',4；网格类
    plot3dRows*plot3dCols==N 的模型数组本身即行主序摊平——定义曲线弹窗
    数值网格态装配口径，下标序即摊平序无需重排）；四钮（添加行/删除末行/
    清空/导出表格）保持置灰（20261117 用户拍板，行编辑/导出留待后续单）；
    表格控件构造期保持启用（置灰调色板会吞行高亮主题蓝——备案），其余
    交互控件未选中全禁用；三列导航按 mPointSeekCore::coordinateVaries 启停
    （恒值列整列灰含 ∩/U 组，复现用户截图 Y 恒值列全灰态）；寻点动作收口
    seekTo=更新当前点{curveId,pointIndex}→工厂 setPointSeekMarker 转发画布
    标记→表格当前行 QTableWidgetItem 字体加粗+主题蓝前景（curveHighlightBlue
    单一色源；不用 selectRow 免引入编辑语义）；换曲线=标记与高亮清除。
  - **工厂寻点状态**（mCurveViewportFactory，全局单标记）：成员
    {_pointSeekChartId,_pointSeekCurveId,_pointSeekPointIndex}（-1=无）+
    公开 setPointSeekMarker(chartId,curveId,pointIndex)（非法入参归一无标记/
    同值守卫幂等/跨图寻点旧图已物化视图先清标记）→qobject_cast<m3DChartView*>
    调其 setSeekMarker（非三维视图安全短路）；新物化三维视图 materializeChart
    内回放现状（R46 注释模式同款承接先例，晚物化/LRU 回收再物化标记不丢）；
    AboutToBeRemoved 联动=标记所在曲线/图表被删即清全局标记（state-only
    收口，目标视图未物化同样生效）。寻点标记为**视图态**：不进命令栈、不写
    文档、不序列化、不触发 rebuild。
  - **画布寻点标记**（m3DChartView::setSeekMarker(curveId,pointIndex)）：
    SeekMarkerOverlay 视图子控件覆盖层（非画布子控件——R32 图例覆盖层/R42
    注释项先例，rebuild 整画布换代不受影响；懒创建首次寻点才建；鼠标穿透
    WA_TransparentForMouseEvents 相机交互零影响），绘制=橙点半径 4px+1px
    深橙描边+坐标文本 "X:%.3f Y:%.3f Z:%.3f"（'f',3 无空格对齐用户截图，
    深灰 #333）；标记数据坐标**文档现读**（不缓存数据副本——按 curveId 取
    三列第 pointIndex 点，仍属本图表校验防跨图串标），repositionSeekMarker
    四路汇聚重投影（setSeekMarker/resizeEvent/画布每帧 Paint 后既有重摆通道
    [layoutAnnotationItems 同一 Queued]/rebuild 尾照常重放——视角旋转/缩放
    随动）；投影失败（相机身后 w≤0/画布未建）或点失效（曲线删除/下标越界/
    任一坐标非 finite）收场隐藏不崩不闪。
  - **同轮勘正（T47-1 复核发现）**：SeekMarkerOverlay/SnapMarkerOverlay
    （R46 问题4 吸附高亮）paintEvent 均按**画布坐标**绘制而 setGeometry 后
    画布原点已平移——橙点/吸附圆环整体错位（吸附环 16×16 内恒裁剪不可见）。
    修复=paintEvent 改按本项局部坐标（寻点标记 _anchorLocal=画布锚点-包围盒
    左上角；吸附环恒几何中心 (8,8)）。R46 吸附高亮自此实际可见。
  - **回归**：workspace tools/pointseektest 新建（QCoreApplication 纯离屏，
    仿 annotest 模式）58 断言 ALL PASS（全局最值并列取首/NaN 断口不入候选/
    局部峰谷端点单侧+平台值不判峰+邻 NaN 按侧不存在/三列步进序含等值稳定
    序/止于端点不环绕/未寻点落序首尾/恒值列判定含 NaN 跳过/网格行主序摊平
    序/空数据与越界安全）。annotest 46 断言 ALL PASS（F 段适配 R46 口径：
    注释管理组退役普查+占位普查恰 {U}{V}{W}+附着方式置灰"三维"+C 段快照
    V14 版本号与 V13 手构容错框位缺省 NaN+C4d V14 NaN 往返）；3dtest 全套
    件 ALL PASS（tst_3dchain 含 D2 空盒红像素阈值 20→30 再校准——R45 引擎
    轴向重绘后刻度数字字形红像素基线 23，探针定位=三轴刻度数字位置非曲线
    内容；datum/r38 快照版本断言 10→14）；r37test 42 断言 ALL PASS（A16
    数值表 4 列/B1b 数值页仅数据表控件启用普查适配 R47 口径；C 段适配
    20261117 曲线样式页勘正三期口径[类型视图下拉恒可用/按钮行1 选中集
    语义/外观普查豁免行摘除隐藏行/C8 改类型视图切换]）；r37qa 12 断言 ALL
    PASS（QA-A1 适配 R41 基准列表实时重载未绑定=空）；ctest 15/15 全过
    （历史"PostWindowTests/PostWindowIsolation 两失败基线遗留"随运行时环境
    修复转绿）。**环境修复备案**：MeshGeoDatad.dll（RebuildBasicData 侧
    20261116 重建）新增静态依赖 RobustPredicatesd.dll，workspace 全部
    tools/*.cmd 运行 PATH 已补 `conan2/p/b/robused6c1e43ca296/p/bin`（缺它
    =GUI 测试链 0xC0000135 全灭，QCoreApplication 套件不受影响）。
    **遗留**：r34test（R34 注释页行集套件）尚有 R46 退役口径适配债未清
    （R46 备案项，本轮已清 annotest 等价面，r34test 待后续单）；产物同步
    bin/Debug→install/bin 全量 cmp 45 件零残差（Testing/ffmpeg/materials/
    python 四目录为本地运行资源历来不入 install）。
  - **T47-2 数值页寻点对齐修正（20261117 用户实机反馈，两口径拍板）**：
    - 极值列首行 ◄► 步进序改**极值点集切换**（原=全点下标序步进）：E=
      局部峰∪局部谷（判定口径不变：严格大于/小于两邻、端点单侧、邻 NaN
      按该侧不存在、仅有限点）按下标升序，◄=E 中小于当前下标的最大者/
      ►=大于当前下标的最小者（当前∈E 即取前后邻居）；当前下标无效（无
      选中/越界/NaN 断口）► 取 E 首个、◄ 取 E 末个；越头/尾**止于端点
      不环绕**。核心新增 extremumPointSet（峰∪谷并集去重升序），stepSeek
      极值列分流改在 E 上切换（seekOrder 极值列全点序保留=坐标数据表行序
      /网格摊平序口径，不再作步进序）；∩/U 六钮与 X/Y 列步进序不变。
    - Y 轴列禁用口径改**曲线类型固定禁用**（替代 T47-1 恒值推断口径——
      设计稿与截图中的曲线 Y 列灰即此口径）：选中曲线 plot3dType==1（三维
      曲线）→ Y 列整列灰（首行 ◄►+∩/U 组一并，即便该列数值全程变化）；
      plot3dType 0/2/3（瀑布/曲面/柱状）与其余列（极值/X轴）维持恒值兜底
      判定。核心新增 columnEnabled(plot3dType,column,...) 三列启停单一
      口径，页面 refreshButtons 按类型分流（类型经文档节点 series.plot3dType
      查询）。
    - 回归：pointseektest 扩至 91 断言 ALL PASS（极值点集步进：当前∈E/
      ∉E/无效下标落 E 首尾/端点止步不环绕/NaN 邻接/单点峰谷去重/全平台
      无极值返回 -1；类型分流：曲线 Y 固定禁用即便 Y 变化/0/2/3 恒值兜底
      可用/X·极值列不受影响；既有全点步进断言按新口径改写）；annotest 46
      断言/r37test 42 断言复跑 ALL PASS（本改动不触渲染，零影响）；ctest
      15/15 全过基线不劣化。
  - **T47-3 数值页寻点对齐修正二（20261117 用户三项拍板）**：①∩=全局最高/
    U=全局最低与三列"每列管各坐标"（X ◄∩►∩/◄U►U=X 自身局部峰/谷）经拍板
    确认与现状一致**不改**；②唯一净改动=曲线类型（plot3dType==1）X 轴列
    首行 ◄► 步进改**按序号**（有效点下标升序，跳过非 finite，止于端点不
    环绕；原=X 升序稳定排序），瀑布/曲面/柱状维持 X 升序——stepSeek 增
    plot3dType 类型感知参数（与 columnEnabled 同风格，核心内分流
    validIndexOrder 复用），页面步进钮传 series.plot3dType；极值列极值点集
    移动/Y 列禁用/其余全部零改动。回归：pointseektest 扩至 102 断言 ALL
    PASS（曲线 X 列序号序 vs 升序序在 X 非单调数据与等值网格数据上双口径
    区分断言/端点止步/NaN 跳过/无选中落序首尾）；annotest 46/r37test 42
    复跑 ALL PASS；ctest 15/15 全过。
  - **T47-4 数值页导出表格 CSV+寻点表格滚动联动（20261117 用户两需求）**：
    - 导出表格钮接线：选中曲线且数据非空启用（与三列启停口径独立——恒值/
      断口数据同样可导出），其余三钮维持置灰；保存框（getSaveFileName 过滤
      器 CSV，默认名=曲线名.csv 剔除 Windows 文件名非法字符 \ / : * ? " < >
      |，取消=空路径零副作用）；CSV 装配=mPointSeekCore::exportTableCsv
      纯函数单一实现（表头"横坐标,纵坐标,高度"+三列数据**无序号列**，
      'f',4 同表格显示，非有限值空串对齐定义曲线弹窗数值表口径，下标序=
      表格行序——网格类行主序摊平同序，CRLF 行尾）；写出=UTF-8 BOM
      （QTextStream setGenerateByteOrderMark，Excel 直开中文表头不乱码——
      mChannelMapCore 先例编码/分隔符对齐；**二进制模式写**规避 Text 模式
      "\n" 再翻译叠加 "\r\r\n"（mDynStiffnessMapping 先例勘正口径），换行
      显式 CRLF）；写失败警告框不崩溃。CSV 装配断言入 pointseektest。
    - 寻点表格滚动联动：全部寻点动作（seekTo 统一收口——极值列 ◄►/∩/U、
      X/Y 列 ◄►/∩/U）更新当前点后 scrollToItem 当前行（EnsureVisible 最小
      滚动）；refreshAll 尾/refreshTable 无当前点（换曲线/清除）滚回顶部，
      有当前点（初始选中带点/数据改写保留点）滚至当前行；高亮机制不变。
    - 回归：pointseektest 扩至 108 断言 ALL PASS（CSV 装配：表头/无序号列/
      'f',4/非有限空串/下标序/空数据仅表头/CRLF 收口）；annotest 46/
      r37test 42 普查复跑 ALL PASS（未绑定态导出钮仍置灰，普查零适配）；
      ctest 15/15 全过。


- **三维图轴线取边勘正——背景面接缝星形+贴轴数字+正等轴测默认视角（R45，
  20261116——引擎单；用户实机截图 vs 设计稿对照，"不是颜色的问题，是轴的
  问题"）**：R44/T44-3 补绘的三根轴线脊线与刻度数字呈"拐角"形贴在包围盒
  外轮廓边上（设计稿为三轴自视口中心交汇角放射的"星形"，差距巨大）。
  根因：`Axis3D::getDrawTicks`/`rebuildGeometry` 脊线/`getAxisTitlePos` 三处
  取边式各多一个取负（X:(·,-fixedY,fixedZ) 等），钉到了 kFixedBackdropFace
  三背景面（X_Max/Y_Min/Z_Min）**对面的外轮廓盒边**（X_Min/Y_Max/Z_Min 组
  的公共角），而填充/网格画的是背景面本身——轴线与背景面接缝错位两套边。
  引擎三改（Plot3D 独立仓 wyf_dev）：
  ①**取边勘正**：三处去取负，脊线/刻度/标题中点统一钉到三背景面两两相接
  的**接缝盒边**（X:(·,fy,fz)/Y:(fx,·,fz)/Z:(fx,fy,·)），自远端交汇角放射，
  刻度数字随缝贴线；
  ②**标签布局重写（Text3DOverlay）**：删 8px 外挑刻度短线（设计稿无刻度
  线）；外挑方向改按 3D tickDir 投影（旧"相对屏幕中心朝外"判定在贴缝后退
  化——缝中点投影在视口中心附近 dot≈0，方向随数值噪声翻转）；数字以 6px
  间隙贴线居中，标题钉 `getAxisTitlePos` 缝中点 16px 偏置；新增
  `getAxisLabelDir/getAxisTitleDir`（Z 缝标签取 -Y 侧=数字在竖直线右侧、
  标题取 +X 侧=标题在线左，对齐设计稿）；
  ③**默认视角正等轴测**（CameraController::reset az -25.5°→-45°、
  el 30°→35.264°=asin(1/√3)）：视线沿盒对角线，交汇角投影恰在视口中心，
  X 右下/Y 左下各 30°/Z 竖直，对齐设计稿对称星形；视点象限不变
  （(-X,+Y,+Z)），kFixedBackdropFace 选择不受影响。宿主零改动（仅消费相机
  矩阵/不覆写方位角；Axis3D.h 增两方法非虚非数据成员、对象布局不变，宿主
  无需重编译）。回归：引擎仓新离屏自检 `.zcode/axis3dcheck`（WA_DontShowOnScreen
  真离屏，桌面合成干扰会产生青色伪影抓帧不可信——实测教训）：三维框/线框
  两场景像素断言 ALL PASS（P1 视口中心 ±10px 轴线蓝≥38/交汇角居中；P2 Z 缝
  中列 200px 段蓝≥366/中列即 Z 脊；P5/P6 自中心 ±6° 扇扫 X 右下/Y 左下射线
  命中率 1.00/0.99/星形方向；线框 fill=0 grid=0 关断保持、三维框 fill=4678
  grid=593 在）。**Plot3Dd.dll md5 `9d449f92` 三处一致**（Plot3D install/bin
  →RebuildGUI bin/Debug→install/bin）+Axis3D.h 头副本同步。
  **遗留（测试仓不在本机，须在宿主测试工作区同步）**：axis3dtest 脊线像素
  断言（T5d/T5e/T5f 按 (xMin,yMin,·) 旧边投影→改 (xMax,yMin,·)）、R44 像素
  基线（spine 计数 71、"相机矩阵逐位不变"断言=相机默认已改、p4mirror 非白/
  饱和/色种）、R5 精确红计数需重整；交汇点三轴最小值数字叠写（"0.0000"团）
  与设计稿行为一致（参考软件 -3 同样叠写）未做避让。

- **三维图网格样式翻活——三维框/线框与背景面配色（R44，20261116——T44-1
  引擎单+T44-2 宿主单串行；R31 占位"网格与外观"组三行转正）**：三维图
  （chartType=5）轴壳参数栏页「网格样式/网格线颜色/背景颜色」三行翻活，
  用户拍板留痕：**三行全活且均为图表级**——不随「应用到」扇出（三键不进
  R43 commitScoped 扇出键集），任意轴壳页编辑全图生效；线框期「显示网格」
  行置灰、两色行保持可用（休眠状态）；轴颜色行本轮仍置灰占位（引擎无轴线
  配色 API，另行需求）；不复用二维 chart.backgroundColor/gridColor 字段
  （默认值语义冲突，三维用新字段保证默认观感零变化）。三落点：
  - **引擎（T44-1，Plot3D 独立仓 wyf_dev 0c9e642）**：`Axis3D` 背景面填充/
    网格颜色硬编码局部常量（draw() 内 0.94 灰底/0.75 灰网格）成员化为
    `m_backdropFillColor/m_backdropGridColor`，默认值=硬编码逐位等效（默认
    观感零变化）；新增 `setBackdropFillColor/setBackdropGridColor(QColor)`
    两 setter（分量 qBound 防越界后 /255.0 转 QVector4D、alpha 恒 1.0 背景
    面不透明语义不变、非有效色按默认值兜底）；成员绘制期消费（draw 内
    setUniformValue），不重建 GPU 缓冲。**线框模式不加引擎 API**——不动
    kFixedBackdropFace/深度语义/纯背景不写深度逻辑/网格几何，宿主复用既有
    setGridVisible 关断语义（T44-3 修订：该关断语义已解耦为"仅网格"，
    原注释行号漂移轨迹 294→322→347，勘正评审建议 1，见下方修正段）。
    Plot3Dd.dll 三处一致（Plot3D install/bin→RebuildGUI bin/Debug→
    install/bin，md5 e8c00505 逐字节，头副本 install/include 同步）。
  - **模型**（RebuildGUI 1a5a54644）：`CurveChart3DData` 增三字段（默认=
    现观感等效 `gridStyle=0`（0 三维框/1 线框）/`backdropColor=(240,240,240)`
    （0.94×255≈239.7→240 8bit 帧缓冲像素同值）/`backdropGridColor=(191,191,191)`
    （0.75×255≈191.25→191 同值））；CurveProp 新键 `kChart3DGridStyle/
    kChart3DBackdropColor/kChart3DBackdropGridColor`（图表级，仅 Chart 节点
    放行，命名 chart3d 前缀区分轴级 kAxis3D*）；Chart 节点读写分支对齐
    kDatum3DVisible/kAnnotationsVisible 先例，写侧 gridStyle 不钳制（页侧
    保证 0..1，沿既有键口径）；序列化 **V9→V10/模板 V3→V4**：writeChart3DAxis
    逐轴段后恒追写图表级三元组 `qint32(gridStyle)<<backdropColor<<
    backdropGridColor`，readChart3DAxis 在 R43 双参门控机制内**再套一层阈值**
    （会话 (version,9,10)/模板 (version,3,4)——不达标整段跳过三字段保持默认
    零回归；达标读入 gridStyle 越界钳 0..1）；V9 及更早快照与 V3 及更早模板
    读入三字段=默认值零回归（旧 .mxses/旧模板零损伤兼容红线）。
  - **页面**（a28592eff）：网格样式行 frozenCombo 翻活两选项「三维框/线框」
    （对齐 HyperGraph Grid Style Box/Wireframe=用户截图图3）；两色行
    frozenColorButton 翻活 `mCurveColorButton+setBarMode(true)`（R34 先例
    通栏色条=截图形态）。三行经新私有助手 `commitToChart`：守卫同基类
    commit（_loading/_nodeId<0 静默），提交目标=`doc->chartNodeId(_nodeId)`
    （<=0 静默返回）经控制器 setNodeProperty 走命令栈；loadFromModel 阻塞
    信号回显（样式 qBound 0..1 与序列化读侧同口径/两色无效回退出厂缺省）。
    线框联动：gridStyle==1 时「显示网格」整行置灰（R33 防假编辑先例——
    引擎旗标被线框语义压住，切回三维框恢复可用并按各轴模型值生效）；两色
    行线框期保持可用（休眠——值继续提交入模，切回恢复消费）。
  - **渲染**（1abe3b9e2）：rebuild 轴组段 setGridVisible 改经匿名 namespace
    `applyBackdropStyle` 单一实现（与增量分流共用禁止两处分叉）——
    gridStyle==1 线框=三轴全关（仅轴线/刻度/数字=Text3DOverlay 独立绘制
    不受影响）；三维框=按各轴模型 gridVisible 原样；并恒下发两色（引擎
    setter 成员消费代价为零，缺省两色=原硬编码等效观感零变化）。
    nodePropertyChanged 分流新增图表级样式分支：判定序在注释分流之后、
    rebuildRelevant 之前，本图表节点×三键命中→可见期重算旗标+两色 setter
    +update()（引擎 setter 均绘制期消费，相机不丢=保视角增量刷新，R33 先例）
    后 return；**rebuildRelevant 白名单不追加三键**（防双触发，R33 口径）；
    不可见期 scheduleRebuild 挂起（B03 口径）；kAnnotationsVisible 既有注释
    分流不动。
  - 回归：workspace `tools/axis3dtest` 扩至 **141 断言 ALL PASS**（R43 口径
    更新[占位普查 14→11/色钮分色定位]+新增 T 节 19：三行可用性/提交目标=
    图表节点探针（sig_nodePropertyChanged nodeId==chartId）+撤销重做/全部轴
    档不扇出（仍图表级单条命令）/两色提交 undo 还原+回显阻塞/线框期显示
    网格置灰切回恢复/两色线框期休眠可用/会话 V10 往返（scaleType+三键
    存活）/V9 手构块 scaleType 存活+三字段缺省/V8 手构块全缺省/模板 V4
    往返/V3·V2 手构模板缺省/像素断言：缺省 fill240=90522·grid191=1025
    （240/191=引擎硬编码等效=缺省观感零变化实证）/红底 fillRed=91239+原
    240 归零+网格保留/网格绿增量 1025 像素+**相机矩阵逐位不变**/线框
    fillRed 91239→717（残余=曲线自身同值红像素）+gridGreen=0+曲线红 725
    仍在/切回恢复且 X 面按各轴模型值关断（fillRed=71760<91239）]+R43 三套
    版本断言按 V9→V10 升版口径更新（annotest C0/3dtest datum·r38 F1b，
    R38 更新 R33 断言同款先例）。**全套件全量重建后 ALL PASS**（3dtest：
    tst_3dchain/curve3dpage 42/datum 60[版本断言 V10]/r33qa 15/r36 48/r38 50
    [版本断言 V10]/r38qa 18/qwa P2·P3 fail=0/**p4mirror 非白=106230
    饱和=82792 色种=60 与基线逐位一致**[引擎成员化+宿主缺省两色=缺省像素
    零回归双实证，T44-1 后复验+T44-2 后复验两次一致]）+axis3dtest 141+
    annotest 50[版本断言 V10]+r34test 84+r37test 42+datumplanetest 全过+
    define3dtest 六套 117/166/110/114/113/57+ctest 8/10 基线一致
    （PostWindowTests/PostWindowIsolation 两失败系基线遗留）。bin/Debug↔
    install/bin 全量 cmp 同步零差异件。
  - **实机修正（T44-3，20261116 同日——引擎 wyf_dev 59a3edd+宿主 640cbef1d，
    用户实机截图三张+设计稿对照两根因）**：
    ①**线框缺轴线（框）**：引擎从未绘制轴脊线（Text3DOverlay 仅刻度短线+
    文本），切线框后只剩悬浮刻度。补绘=Axis3D::rebuildGeometry 增三根轴线
    脊线几何（独立 `m_spineRange`，追加网格线 VBO 尾段；取边=kFixedBackdropFace/
    getDrawTicks **同一映射常量**：X 脊 (·,-fixedY,fixedZ)/Y 脊
    (-fixedX,·,fixedZ)/Z 脊 **(-fixedX,fixedY,·)**——三条盒边），draw()
    纯背景通道内**恒定绘制**——不 gate 于网格显隐/填充开关，两样式+显示
    网格任意组合下轴线都在（线框观感=三根轴线+刻度+数字=用户图2）；颜色=
    新成员 `m_axisSpineColor`（默认参考软件轴线蓝 0.29/0.51/0.85≈8bit
    (74,130,217)），系后续「轴颜色」行接线挂点（当前无 setter 恒默认）；
    Text3DOverlay 零改动。
    ②**显示网格语义过宽**：原"网格关=该轴面填充一起不画"（用户实机：三维框
    +显示网格不勾连背景面也消失）。新语义（用户拍板，用户图3）：**显示网格
    只管网格线**（关=仅网格线不画）；背景面填充由引擎新 setter
    `setBackdropFillVisible(bool)` 管（新成员 `m_backdropFillVisible` 默认
    true=观感兼容）；**轴线恒绘不受任何开关影响**。宿主 applyBackdropStyle
    单一实现改：三维框=fillVisible(true)+各轴模型网格值；线框=fillVisible
    (false)+三轴全关；两色调用不变（rebuild/增量两调用点自动同享）。
    模型/页面零改动（键不变、线框期「显示网格」置灰维持）。
    **像素基线重整（旧→新留痕，非静默改数；两轮演变）**：默认三维框观感
    新增三根轴线=预期变化（对齐参考软件）。
    第一轮（59a3edd，Z 脊错边态）：p4mirror 非白 106230→106230/饱和
    82792→83124/色种 60→83；axis3dtest T5a 缺省 fill240 90522→90328/
    grid191 1025→967/spine=273；X1"隐藏≤20"→"隐藏<彩显/2"；R5 相等性改
    **精确红**（countRed 含 AA 边缘混合像素，轴脊改变曲线边缘 AA 混合背景
    致恰一像素跨阈值 237,37,124→200,0,87，诊断实证后修订，曲线几何/范围
    语义不受影响）；r38 R1c/R4 稀疏比 1/3→1/2（轴脊非白增量两态同加吃掉
    旧余量，"远稀于"意图保留）。
    第二轮（f95e8d2 评审返工，Z 脊取边勘正）：
    **评审退回缺陷=Z 轴脊线取边错边**——Z 脊 y 分量误写 -fy(=+S)，而
    getDrawTicks/getAxisTitlePos 的 Z 边 y 分量均为 fixedY 正号(fy=-S)，
    刻度/数字/标题钉在 (-S,-S) 边、脊线却画在 (-S,+S)，默认视角投影为
    穿过背景面填充区中央的竖线（评审岗三指纹：T5a Δfill≈-194=脊线整列
    覆盖填充/p4mirror 非白不增/r38 白底态 +303 同向）；系 X/Y 模式套抄时
    误把"取负"复制到 Z 的 y 分量。一行符号修正+注释勘正；CurveReadMe
    本段"取边"表述随勘正（Z 边=fixedY 正号）。像素基线随新观感再整——
    p4mirror：饱和 83124→**83523**/色种 83→**86**（非白恒 106230）；T5a
    缺省 fill240 90328→**90522**（=无脊基线还原：脊线移出填充区，退回
    盒缘与既有网格边界线共线）/grid191 967→**958**/spine 273→**71**；
    T5d 线框 spine=71；T5e 填充不减 94814≥91239+网格 641<958。
    **轴线计数口径备案**：Text3DOverlay 刻度标签背景矩形在离屏抓图时擦除
    GL 像素（R3 先例扩展：文字不进帧缓冲、其背景矩形会）——脊线在标签位
    呈周期缺失（tickCount=6 实测 218px 跨度余 71px/14 段），真实应用标签
    处绘制文字不受影响；axis3dtest 脊线计数阈值按侵蚀后余量取（>40）。
    **位置性断言（返工新增，防"计数验存在不验位置"漏网）**：T5f——经
    projectDataToPixel 投影 Z 刻度贴边 (xMin,yMin,zMid)（盒 (-S,-S,0)，
    与渲染归一化同一真源），断言该列 ±2px 存在轴脊蓝（=14）且首版错边
    (-S,+S) 投影列中段 ±2px 零轴脊蓝（错边列中段=首版穿填充竖线所在，
    现零命中；该列下端为 X 脊合法端点角点，故负向只查中段行带）。
    回归：axis3dtest **145 断言 ALL PASS**（首轮 142+返工 T5f 三断言；
    T5a/T5d/T5e 脊线阈值按标签侵蚀口径重整）+3dtest 全套（3dchain/
    curve3dpage 42/datum 60/r33qa 15/r36 48/r38 50/r38qa 18/qwa fail=0）
    +p4mirror 上列新值+annotest 50/r34test 84/r37test 42/datumplanetest
    全过+define3dtest 六套 117/166/110/114/113/57+ctest 8/10 基线一致
    （PostWindowTests/PostWindowIsolation 两失败系基线遗留）。
    Plot3Dd.dll 三处 md5 `5da22057` 一致（首轮 f58eabd9→返工 5da22057）
    +头副本同步；bin/Debug↔install/bin 全量 cmp 同步零差异。

- **三维轴参数栏三行翻活——坐标轴切换/应用范围/刻度类型（R43，20261116——
  T43-1；R31 占位三行转正收官）**：三维图（chartType=5）轴壳参数栏页
  `mAxis3DPropPage` 翻活三行，四项用户拍板留痕：①「应用到」生效键范围=**仅
  范围与刻度类五键**（kAxis3DScaleType/kAxis3DMin/kAxis3DMax/kAxis3DAutoFit/
  kAxis3DTickCount），轴名称/显示网格**恒单轴不扇出**；②「坐标轴」切换经
  setCurrentNodeId **联动树选中**（R37 坐标轴按钮 mPostMenuContributor 同款
  链路——树回选/参数栏换页/工具页联动免费获得）；③刻度变换 **ref 恒 1.0 纯
  dB**（三维页无参考值行，模型不设 dbRef 字段）；④刻度类型下拉**英文文案与
  顺序=二维轴页"缩放方式"同款**（Linear/Logarithmic/dB20/dB10）。三落点：
  - **模型**：`CurveAxis3DState.scaleType`（0 Linear/1 log10/2 dB20/3 dB10，
    值表与二维 CurveAxisData::scaleMode 同表，缺省 0 旧数据零影响）+
    CurveProp 新键 `kAxis3DScaleType`；壳重定向读写两段各加一行（写侧沿
    既有键口径**不做钳制**——页侧保证 0..3，R38 评审备案口径）。**会话快照
    V8→V9**：writeChart3DAxis 末尾逐轴恒追写 qint32（会话/模板两路径共用），
    readChart3DAxis 增"版本+阈值"双参（会话传 (snapshotVersion, 9)、模板传
    (version, 3)——两路径版本空间独立由调用点语义决定），达标才读、越界值
    钳 0..3 防脏数据（对齐 V5 drawStyle 先例）；**模板 V2→V3**；V8 及更早
    快照与 V2 及更早模板读入缺省 0 线性零回归（旧 .mxses/旧模板零损伤兼容
    红线）。
  - **页面**（mAxis3DPropPage）：①坐标轴三选项下拉（构造期一次 addItem，
    loadFromModel 阻塞信号回显壳下标——不再逐次 clear/addItem；用户切换按
    doc->parentNodeId 找本曲线、childNodeIds 找目标类型兄弟壳命中则
    setCurrentNodeId，回填期 _loading 守卫静默、异常骨架静默返回 combo 由
    提交回环回正）；②应用到 所选坐标轴/全部轴——**会话级 UI 态不进模型不
    序列化**（文件局部静态档，三实例[参数栏按壳类型缓存、同时存活]共享，
    切换只改档不提交、loadFromModel 阻塞回显）；新私有助手 `commitScoped`：
    所选坐标轴或键不在五键范围→基类 commit 原样，全部轴且键在范围→控制器
    beginMacro("修改三维轴") 包本曲线三兄弟壳逐个 setNodeProperty（壳缺失
    降级为可用壳集合、本壳保底），endMacro 收口一次撤销整组还原；③刻度类型
    四项提交 kAxis3DScaleType 0..3 经 commitScoped，回显 qBound(0,v,3)+阻塞
    信号。`commitManualRange` 全部轴档=宏内逐轴提交**各自的**当前生效范围
    （queryAxis3DDisplayRange 出参按轴写回对应壳）+autoFit=false，查询失败
    =仅三壳退出自动回退休眠值。min/max 互钳保持本页控件局部行为（扇出值
    直接提交，各轴页面经提交回环回填自紧）。占位 11 行纪律不变（构造即禁用、
    不连 commit、不进回填）；reloadForUnion 全局监听零改动（兄弟壳属性变化
    =图表后代事件已被覆盖）。
  - **渲染**（m3DChartView，宿主侧数据预变换+线性轴策略，**引擎零改动**）：
    变换单一实现复用 `mCurveChartView::transformValue/inverseTransformValue`
    公开静态（dbRef 形参缺省 1.0=拍板口径；变换返回 NaN[原始值 ≤0 或原
    NaN]该分量置 NaN=对齐二维"非法点剔除"断点语义），禁止复制公式。rebuild
    经匿名 namespace `axis3dRenderData` 单一实现（rebuild 系列下发/轴组范围
    与 refreshDisplayRangeCache 缓存镜像共用）：任一轴刻度非 0 → 逐曲线建
    变换副本（线性轴浅共享零拷贝=**快路径零回归**；副本仅用于 setData/统计，
    模型不动）+ 变换空间并集（对副本跑 unionAxisCore——unionAxisRange 抽取
    向量组核心单一实现，原始/变换/过滤三路径共用；非有限整点排除先例自动
    生效[变换 NaN 点不入统计]、退化兜底 max+=1 在变换空间）+ 生效范围
    （effectiveAxisRanges 单一实现：手动值按轴刻度变换，**变换 NaN 单侧
    兜底=该轴变换后并集对应侧，并集不可得=线性缺省 0/10 再退化兜底**——比
    二维条带 0..10 兜底更优，备案差异点）；setCoordinateLimits/axis setRange
    全部下发变换空间值，labels/tickCounts/gridVisible 不变（刻度数字=变换
    空间值，对齐二维口径）。`queryAxis3DDisplayRange` 契约保持**原始数据
    空间**（属性页回显/退出自动回写依赖，零 UI 改动）：并集统计前按"整点可
    变换"过滤（任一分量在其轴非 0 刻度下变换 NaN → 整点排除，镜像非有限
    整点排除先例），返回存活点原始值并集；线性轴过滤恒真零变化。显示范围
    缓存=**变换空间**（引擎归一化真源，与 setCoordinateLimits 同一实现同一
    取值）。注释锚点（kPosX/kPosY/kPosZ 恒存原始值）：投影先按轴变换再入
    盒[锚点变换 NaN → 投影 false 项收场隐藏防御]、`unprojectPixelThroughData`
    逆路径末端 inverseTransformValue 回原始空间再落键——**R42 拖拽闪回修复
    逻辑（差分法/显式 QVector3D 构造）原样保留仅外围包变换**，正逆变换闭合；
    基准面位置按法向轴刻度变换后 setPosition（NaN 跳过该面防御）。**色带
    值域口径不变**（contourValueRange 与 rebuild 色带回落仍用原始 z 并集，
    已知限制备案：刻度类型不改色带值域）。rebuildRelevant 白名单追加
    kAxis3DScaleType（数据预变换走全量重建）。
  - 回归：workspace `tools/axis3dtest` 扩至 **117 断言 ALL PASS**（原 75
    断言按翻活口径更新[三行定位改选项集式/占位普查 16→14/身份下拉可用]+
    新增 42：三行行集回显阻塞/刻度类型提交撤销重做/坐标轴切换
    sig_currentNodeChanged 探针命中兄弟壳+非法下标静默/应用到档位跨实例
    共享不进模型/全部轴扇出=宏一条[index delta=1 口径：前序 undo 的 redo
    段截断使 count 净变化可为 0——P7 备案同款，index 不受截断影响]/一次
    undo 三轴整组还原/轴名称·显示网格恒单轴/min·max 扇出与互钳局部/取消
    自动适配全部轴逐轴写回各自现值+查询失败仅退自动/壳重定向写读兄弟壳
    互见/写侧不钳制+页回显钳制/会话 V9 往返/V8 手构字节块缺省 0/模板 V3
    往返/V2 手构模板缺省/log 轴整点可变换过滤并集/含非正值离屏渲染不崩/
    投影∘反投影恒等闭合+拖拽像素闭合+NaN 锚点防御/线性恒等对照）。
    **全套件全量重建后 ALL PASS**（3dtest：tst_3dchain/curve3dpage 42/
    datum 60/r38 50[两套件快照版本断言按 V8→V9 升版口径更新——R38 更新
    R33 断言同款先例]/r33 42/r33qa 15/r36 48/r38qa 18/qwa P2·P3 fail=0/
    p4mirror 非白=106230 饱和=82792 色种=60 与基线逐位一致[线性轴像素
    零回归实证]）+ annotest 50[C0 版本断言同步 V9]+r34test 84+r37test 42+
    datumplanetest 全过+define3dtest 五套 117/161/110/113/57+ctest 8/10
    基线一致（PostWindowTests/PostWindowIsolation 两失败系基线遗留）。
    Plot3D/PlotView 引擎零改动。

- **三维基准面一期管理闭环（R41，20261113——T41-1/T41-2/T41-3 三单串行；"点
  新增→3D 视图立即出现默认基准面+管理列表加条目+一次撤销"为验收口径）**：
  Ribbon"3D曲线"页签"标注"组"基准"按钮（R37）挂载的 mDatum3DPage 从纯占位
  翻活，基准面成为三维图（chartType=5）下的普通树节点+3D 画布真实渲染。三段
  落点（引擎/模型详见各自实现说明，此处记宿主侧事实链）：
  - **引擎图元（T41-1，Plot3D 独立仓 wyf_dev fe8ad43）**：`DatumPlane3D`
    （Abstract3DRenderable 子类，PLOT3D_EXPORT）——垂直于法向轴、过该轴坐标
    position 的全截面参考平面（数据坐标，内部按 setCoordinateLimits 同源机制
    归一化到盒空间、越界钳制入盒；setBoxExtents 与 setCoordinateLimits 同存
    储便利入口）；渲染=半透明纯色填充+四边边框+面向相机 billboard 文字标签
    （文字色按填充色亮度自动取黑/白），混合纪律与 R36 幕帘同套（blend 开、
    深度写关、深度测试开=前景不透明序列正确遮挡）；**非拾取**（getDataCount()=
    0+drawSelection 空实现，不偏移拾取 ID 段）；全部 setter 仅写成员+标脏、
    draw 前按当前盒重建几何（R36 覆写标脏同款）；默认值=参考软件截图口径
    （法向 X/位置 0/橙 221,147,96/不透明度 0.25/锚点中心/可见）。**多半透明
    图元间无深度排序、按 addItem 插入次序混合（引擎既有行为备案，宿主控制
    插入次序）**。DLL 三处同步（Plot3D install/bin→RebuildGUI bin/Debug→
    install/bin）按 T41-1 预设由本单收口全量 cmp 补齐（同批差异件一并同步，
    见下方回归段）。
  - **模型层（T41-2，RebuildGUI 04930d9b3）**：`CurveNodeType::DatumPlane3D`
    枚举尾部追加+`CurveDatumPlane3DData`（type/normalAxis/posX/posY/posZ/
    text/labelPos/color/opacity/reverseClip/visible，出厂默认=截图口径）+
    `CurveChartData.datum3DVisible`（图表级"显示基准"总开关，默认 true）；
    CurveProp 新键六枚 kDatumType/kNormalAxis/kPosZ/kDatumOpacity/
    kReverseClip/kDatum3DVisible+复用既有键 kName/kVisible/kColor/kText/
    kLabelPos/kPosX/kPosY（分发按节点类型区分）；nodeProperty/setNodeProperty
    全键路由+同值短路+越界钳制（写侧+operator>> 双侧）；控制器
    `addDatumPlane3D(chartNodeId, errMsg)` 三级守卫（id 不存在/非 Chart/
    chartType≠5）+宏"新建基准面"一次整体撤销+创建后不 setCurrentNodeId（导航
    归工具页，与 R34 注释 Ribbon 按钮行为差异备案）；默认名"基准 %1"计数器
    会话内递增不回收（restoreCounters qMax 合并不回退=全计数器共用语义）；树
    集成=词表四件+程序化图标（半透明橙平行四边形）+d3 profile 基准分区（注释
    区之后尾部追加）+右键删除查表自动生效+筛选器不新增分组备案；序列化 V7
    （其被 R42 修订推进为 V8 链条的一环：V6 注释开关→V7 基准面→V8 注释
    posZ，各版本门控/钳制/缺省兜底独立成立）；模板路径零波及（类型上界放行，
    恢复缺省）。
  - **页面翻活+渲染接线（T41-3，本单）**：
    - **mDatum3DPage**（非节点绑定工具页机制不变——不继承 mCurvePropPageBase、
      不进注册表、宿主挂载链零改动）：▾基准管理[显示基准（图表级
      kDatum3DVisible 走命令栈）+名称框（当前选中基准本名，编辑失焦且变更才
      一次提交 kName，空名拒绝防"基准 N"丢失）同行/基准列表=本图表全部
      DatumPlane3D 子节点树序（Qt::UserRole 携带节点 id，勾选=kVisible 即时
      单键提交，单击=选中→属性行回显）/新增=addDatumPlane3D 宏+列表选中新项
      +属性行回显、复制=宏"复制基准面"（嵌套宏并入顶层一次撤销，新节点+逐键
      克隆全部属性、名不克隆续计数、插后选中）、删除=removeNode；按钮启停随
      选中态/列表空态，图表删除兜底=列表清空全钮禁用不崩溃]/▾位置与属性[类型
      （单选项"参考平面"，可用但变更零提交——无第二类型）/法向方向（X/Y/Z 轴
      →kNormalAxis）/X·Y·Z坐标（4 位小数 spin→kPosX/kPosY/kPosZ，口径对齐
      mAxis3DPropPage）/文字（单行编辑，失焦且变更才一次提交→kText，对齐注释
      页纪律）/标签位置（9 宫格下拉→kLabelPos，初显=设计稿默认"中心"）/
      基准颜色（mCurveColorButton 通栏色条→kColor）/不透明度（0..1 双精度
      spin→kDatumOpacity）/反向裁剪=占位]/▾创建截面曲线（四控件占位）。
      占位纪律 R31/R37 逐条（移动增量值框+滑条/反向裁剪/创建截面曲线组构造即
      禁用、不连 commit/信号、不进回填）；回显全程 QSignalBlocker（R30 教训）
      +构造末尾 refreshAll 收敛"无绑定不带电"基线；上下文维护=构造全局监听
      控制器三信号（本图表后代增删/属性变化幂等刷新，图表删除清空兜底）+
      setChart 公开 API——**宿主挂载链零改动口径下由页面自身调用**：构造与
      showEvent（挂载=show）各以工厂 currentChartId() 刷新一次+订阅工厂
      sig_currentChartChanged 跟随三维图切换（仅有效三维图重绑，-1/非三维图
      保持现绑定不冲空），等效 mCurveStyle3DPage 的"挂载前 setChart"；窄宽
      B01/B02（minimumSizeHint 宽 134px 实测，窄 dock 无裁切）。
    - **m3DChartView 渲染接线**：`_datumPlanes`（基准节点 id→
      shared_ptr<DatumPlane3D> 映射，rebuild 开头随 _lineSeries/_surfaceSeries
      一并 clear）；rebuild 遍历本图表 DatumPlane3D 子节点（树序），单基准
      kVisible 且图表 kDatum3DVisible 均为真→建引擎项（**先 addItem 借 GL
      上下文**再下发属性：法向/位置（法向轴对应坐标）/颜色/不透明度/标签
      （text 空取本名）/锚点；盒范围由 scene setCoordinateLimits 统一下发——
      本块位于该调用之前，limits 广播可及）；**插入次序=序列之后**（场景按
      插入次序绘制：轴背景→序列[幕帘]→基准面，半透明混合无排序=T41-1 备案
      ③）；隐藏基准/总开关关闭不建项；**消费侧 qIsFinite 防御（T41-2 评审
      建议④，必做）**：kDatumOpacity 非有限值（脏流 NaN 穿透模型层 qBound）
      回退出厂默认 0.25、位置三键同查回退 0；kDatumType/kReverseClip 本期零
      渲染消费（字段入模保留）。**重建触发**：rebuildRelevant 白名单追加基准
      键集（DatumPlane3D 节点×kName/kVisible/kDatumType/kNormalAxis/kPosX/
      kPosY/kPosZ/kText/kLabelPos/kColor/kDatumOpacity/kReverseClip ∪ Chart
      节点×kDatum3DVisible）——**kName 按节点类型守卫纳入**（仅 DatumPlane3D
      改名触发重建=标签随名刷新；Curve kName 仍由 legendPropRelevant 截获仅
      刷图例覆盖层，不进白名单，防 R32 式冗余重建——像素断言双侧在案）；
      kVisible 语义=T8 既有曲线口径不变、基准节点命中基准键集、注释节点 kVisible
      先被 annotationPropRelevant 截获；结构增删走 R35 既有链路（nodeAdded/
      广谱删除，chartNodeId 对 DatumPlane3D 后代正确返回所属图表——通用父链
      上溯实现，零改动确认）；基准面走全量 rebuild 刷新（面数少重建廉价，不
      做增量样式分支；rebuild 后相机=默认 fit 视角，与曲线增删等全部既有全量
      重建触发同口径）；与 R42 注释画布项共存（注释项为 QWidget 覆盖层不在
      GL 场景次序内，基准面×注释框×幕帘三体同屏=GL 半透明混合+覆盖层叠加，
      回归像素断言+PNG 自证覆盖）。
  - **回归（workspace tools/3dtest/tst_datum，60 断言 ALL PASS）**：页面断言
    （行集三组齐全/占位行禁用+程序化触发撤销栈双零/新增宏一次撤销+redo/复制
    克隆属性+名续计数/删除/列表勾选单键提交/名称与文字失焦提交+无变化零提交/
    法向与坐标与颜色与不透明度提交回显+undo 回显跟随/显示基准总开关/图表删除
    兜底/窄宽防线 134px）；渲染像素断言（离屏 GL：无基准橙系≈0→新增出现橙系
    面像素/隐藏单基准→消失/总开关→全消/法向切 Y/Z 三朝向像素互异/撤销重做
    逐像素复原）；**共存断言（T41-1 评审建议②）**：基准面×幕帘填充
    （Line3DSeries areaFill）×画布注释框三体同屏+基准面×曲面（色带类）同屏
    像素断言，插入次序口径备案（见上），PNG 自证（out/CO_datum_*.png——曲面
    不透明正确遮挡基准面=深度测试保留实证）；序列化断言（当前版本 V8 往返
    无损全字段+重存字节级一致；V5 手构旧字节块读入兼容缺省兜底+计数续接）。
    **既有全套件全量重建后 ALL PASS**（m3DChartView 类布局变更=宿主 ABI 变化，
    旧 workspace exe 按 R36 教训全部重建）：3dtest（tst_3dchain/tst_curve3dpage
    42/tst_r33qa 15/tst_qwa P3 fail=0/tst_r36 48/tst_r38 50/tst_r38qa 18/
    tst_p4mirror 非白=106230 饱和=82792 与基线逐位一致）+axis3dtest 75+
    r34test 84+r37test 42（A14/B1d 两断言按 R41 生效口径更新：列表实时重载
    无绑定=空、仅"类型"下拉可用其余禁用——R38 更新 R33 占位断言同款先例）+
    annotest 50+define3dtest 117/110/113/57+ctest 8/10 基线一致
    （PostWindowTests/PostWindowIsolation 两失败系基线遗留）。**DLL 同步收口
    （本单必做红线）**：bin/Debug↔install/bin 全量 cmp——差异件 9 个全同步
    （Plot3Dd.dll=T41-1 欠账+PostCurved.dll/PostWindowd.dll/MechMainWindowd.dll
    及各自 pdb/exp/lib=并行在途产物），同步后零差异；Plot3Dd.dll 三处
    （Plot3D install/bin、bin/Debug、install/bin）逐字节一致（1,172,480 B）。

### 5.13.2 三维"定义曲线"弹窗（mDefine3DCurveDlg，20261112 首记——a2d6d426 引入，同日类型视图过滤改版；20261113 修订——新增/粘贴默认名跟随条目类型并按类型独立计数；同日修订二——分量下拉恢复列清单口径（撤销矩阵整表特路）+ 弹出高度固定；同日修订三——Z 侧数学来源二元 f(x,y)：网格类瀑布/曲面/柱状 X×Y 全网格求值 rows/cols 实值入档 + 曲线逐点配对求值（一期曲面 R39、同日二次拍板扩展四类型）+ 键盘 y 键按活动向量侧动态启停；同日修订四——数学非有限值断口口径：求值清洗 ±inf→NaN+统计提示+全非法拒绝，引擎破洞配套见 Plot3D 仓 R40；同日修订五——数值页交互对齐二维：[+][-][清空]工具条+右键菜单六项+淡蓝选中/键入编辑，右侧竖排五钮退役，并闭合 B03 数值表编辑无限递归栈溢出；同日修订六——数值页交互对齐二维二期：来源切换转换链（切出前来源内容装配进该侧数值列）+ 非数值侧列只读回显 + 按侧脏标记与离开"数值"确认；20261116 修订六补条——Z 侧数学转换/回显改逐点配对二元按行求值（含 y 表达式可求值；网格类条目不摊平网格）+ 非有限行空串 + U 列空白归一补 0；同日修订七——数值网格态（网格类 Z 数学→数值转换改整网格摊平、维度落条目；应用网格态前置分支按维度直读装配；网格态下数值表三列全部可编辑）与应用原位更新（editCurveId 绑定对齐二维：绑定存活原位更新不新建、粘贴解绑、删曲线重建重绑）；同日修订八——打开载入既有曲线（对齐 2D 编辑模式：showInstance 先复位再载入目标三维图全部曲线为绑定条目、选中曲线/轴壳为初始选中；逐侧溯源还原 X/Y/Z 数学表达式与文件分量列下标，缺席侧当前数据倒数值列兜底+网格维度还原数值网格态；应用溯源落模 set3DCurveProvenance 主标记数学>文件>数值；模型增 z3DExpression 与 x/y/z3DComponent 四字段+会话快照 V13 追写段））

Ribbon"3D曲线"页签 -> "定义曲线"按钮 -> `mDefine3DCurveDlg`（模块根，功能弹窗
单例，非模态白卡片，M徽标标题栏+✕可拖动缩放，`DlgGeom::bind(this,
"Define3DCurveDlg")` 几何记忆）。三栏形态照参考截图：左栏=对象行（"对象"复选框
+ 名称编辑框 + 类型视图下拉[曲线/曲面/瀑布图/柱状图]）+ 待新增对象列表 +
剪切/复制/粘贴/新增；矢量列 X/Y/Z/U/V 单选（活动向量侧）；来源列 文件/数学运算/
数值 单选切右栏三页。单侧定义照 2D 定义曲线的 per-vector-source 模型
（`Define3DSide` 按向量侧独立持有来源/分量/表达式）。

- **类型下拉=类型视图过滤器（20261112 改版）**：列表只显示
  `plot3dType == kTypePlot3d[下拉序]`（固定映射 {1,2,0,3}=曲线/曲面/瀑布图/
  柱状图，对齐 `set3DSeriesData`）的条目，其他类型**隐藏不删除**、切回完整
  恢复（定义数据留存条目结构）；切换后目标类型有条目→自动选中第一条并载入
  右栏，无条目→无选中态（对象行/右栏禁用），**下拉自身恒可用**（驱动过滤与
  新增类型，不随选中态/复选框启停）。条目创建后类型不可经下拉改（下拉不回写
  `plot3dType`，`loadEntryToPanel` 亦不按条目回显下拉——可见条目类型恒等于
  当前过滤类型）。列表项以 `Qt::UserRole` 携带 `_entries` 下标（`_current`
  存条目下标，行号≠条目下标——剪切/粘贴/过滤后的选中维护全走
  entryAtRow/rowForEntry 映射，杜绝行号错位）；程序回填全程 `_syncing` 压信号
  （R30 教训），`rebuildFilteredList` 的 clear/逐项加入中间态不驱动选中链路。
- **首开空列表（20261112 改版）**：首次构造列表为空、无默认"曲线 1"，对象行/
  右栏整体禁用，"应用"（无勾选条目）与"粘贴"（无剪贴板）禁用，类型下拉与
  "新增"可用；条目仅经"新增"/"粘贴"创建。非模态单例，条目会话内留存（关闭
  再开不清空），"首开"仅指 `_instance` 首次构造。
- **新增/粘贴跟随视图类型（20261112 改版）**：`onAddClicked` 新条目
  `plot3dType = kTypePlot3d[当前下拉序]`（曲线→1 曲面→2 瀑布图→0 柱状图→3），
  默认名 `nextEntryName(plot3dType)` 按条目类型生成——前缀与类型下拉文案逐字
  一致（plot3dType 0..3=瀑布图/曲线/曲面/柱状图，注意与下拉序映射 {1,2,0,3}
  区分），"曲线 N/曲面 N/瀑布图 N/柱状图 N" 各类型会话内独立计数
  （`_typeCounters[4]`，剪切/删除不回收；20261113 修订——原 `nextEntryName`
  全局自增"曲线 N"造成"选曲面新增的却叫曲线 1"的误导），新增后过滤列表末行
  可见并选中；粘贴按剪贴板完整定义（含原类型）追加并**按剪贴板条目自身类型**
  重命名（续该类型计数，避免同名歧义），类型 ≠ 当前视图类型时**先把下拉切到
  该类型**（过滤视图随之切换），粘贴条目可见并选中；剪切按
  当前过滤视图重建（只影响被剪条目，其他类型条目不受影响），剪后按被剪条目
  原行位补位选中（越界收敛末行），无同类条目=无选中态。
- **应用（onApply，20261112 改版；20261116 修订七起=绑定原位更新+新建并存）**：
  装配
  **全部勾选条目**（含当前过滤视图外的隐藏类型），逐条目 X/Y/Z 三侧按各自
  来源装配、等长截齐（失败红字带条目名与侧原因，不写文档；网格态前置分支
  见修订七）；单视口定位
  （R30：全文档查 chartType=5，无则"新建三维图"宏 addPage→add3DChart）后
  条目绑定生成曲线（editCurveId）——绑定节点存活=复用原位更新（不
  add3DCurve），无绑定/绑定失效=宏内新建并回写绑定，`set3DSeriesData` 直写
  （plot3dType=条目自身类型；**网格模式 rows/cols=len(Y)/len(X) 实值入档**
  （20261113 修订三），条带模式 rows/cols 恒 0,0（含曲线 Z 侧数学逐点配对
  入档，修订三二次拍板）——逐侧选列装配（20261113 修订二拍板，选哪列=该侧
  数据）按侧装配无网格形状，曲面/瀑布按所选列装配为条带诚实呈现）；成功后
  条目留存可继续修改再次应用（原位更新不重复新建，详见修订七）。
- **Z 侧数学=分类型装配（20261113 修订三，R39 一期曲面；同日二次拍板扩展
  四类型）**：装配分流按条目类型×Z 侧来源三分——
  ①**网格模式**：判定 `entryInMathGridMode`=网格类条目 plot3dType∈{0,2,3}
  （瀑布图/曲面/柱状图）且 Z 侧来源==数学运算（表达式是否真含 y 不作判定——
  常量表达式得到均匀网格自然成立）。装配 `assembleEntry`：先装配 X 轴
  （assembleSideVector kVecX，数学来源仍按 u∈[0,1] 均匀 101 点采样）、再装配
  Y 轴（数学来源采样横坐标仍取装配出的 X），后经
  `mCurveExpressionEvaluator::evaluateGrid` 按 X×Y 笛卡尔积全网格求值
  Z——rows=len(Y)/cols=len(X)，单元 Z(r,c)=f(xs[c], ys[r]) 行主序（行=Y 列=X，
  与"从文件新建三维图"矩阵表口径一致），三轴逐单元扁平展开喂 set3DSeriesData
  （xs[r*cols+c]=X 轴第 c 值、ys[r*cols+c]=Y 轴第 r 值，rows*cols==点数才被
  控制器认作规则网格——渲染端同口径防御）；网格两轴独立不做等长截齐（曲面
  真网格/瀑布矩阵瀑布/柱状柱阵观感；网格分支装配代码二期零改动，仅类型判定
  放宽）。
  ②**逐点配对模式**：曲面条目 plot3dType==1 且 Z 侧来源==数学运算——曲线=
  点列空间线（引擎 Line3DSeries 折线，网格形态渲染无意义），装配序 X→Y→Z：
  X/Y 两轴按各自来源装配后**按行位配对截齐到较少者**（附既有"各侧行数不一致，
  已截齐"note 口径），经 `mCurveExpressionEvaluator::evaluatePointwiseXY` 逐点
  z_i=f(x_i,y_i)（x=装配 X 数据第 i 点、y=装配 Y 数据第 i 点），条带入档
  rows/cols 恒 0,0（表达式不含 y 的 z=f(x) 为该路径的等价形态，数值与既有
  条带口径一致）。
  ③**条带模式**：其余一切组合（Z 侧非数学来源）既有逐侧装配+等长截齐口径
  零变化。
  PlotView/Plot3D 引擎零改动（消费既有真网格入档，R28 已持久化 rows/cols）。
- **键盘 y 键动态启停（20261113 修订三；二次拍板放宽四类型）**：数学页 y 键
  （行1列5）由恒置灰改 `refreshYKeyState` 动态启停——**活动向量侧=Z 即启用
  （不限条目类型）**（启用 tooltip"插入自变量 y（Z 侧=本对象 Y 数据；曲面/
  瀑布图/柱状图按 X×Y 网格求值，曲线按行位逐点配对求值）"——网格/逐点两类
  口径并列说明，点击在表达式光标处插入 y），否则置灰（tooltip"自变量 y 仅
  Z 侧数学来源支持"）；y 语义=**自变量 y（=本对象 Y 数据）**，非向量引用。
  刷新点=选中条目载入/向量侧切换/来源切换三链路（统一收口 loadSideToPanel
  末尾，_syncing 语义下同样刷新——启停属 UI 态不写条目）。求值口径：Z 侧
  数学表达式可含 x、y 双自变量（网格类全网格求值、曲线逐点配对求值）；
  **非 Z 侧**表达式含独立标识符 y 直接报错"自变量 y 仅 Z 侧数学来源支持"
  （不静默——engine "未知变量"文案不点名语义；裸 y 预检在 assembleSideVector
  数学分支只管放行与否（vector==kVecZ 放行），实际求值在 assembleEntry 分流）。
  表达式框占位文字说明两类口径（X 侧 x=u 不变；Z 侧 x=对象 X 数据、y=对象
  Y 数据：曲面/瀑布图/柱状图按 X×Y 全网格求值，曲线按行位逐点配对求值）。
- **分量下拉=列清单口径（20261113 修订二）**：分量行**常显**（任何类型/
  文件形态不隐藏、无说明标签互换），下拉列出文件全部列（首项"未选择"，
  列序=解析缓存列序）。列名规则（`componentDisplayName`，仅展示层改名）：
  `m3DPlotDataParser::isMatrixTable(parsed)` 为真（矩阵表，表头行为数值
  token）时各列展示名恒 `Column N`（N=数据列下标+1，含原本回退名/数值
  token 名的全部列）——不把数值 token 当列名展示，与"曲线部分"（2D 定义
  曲线/无表头 csv 口径）一致；点列文件维持现行口径（表头名或 Column N
  回退名，读取器既有行为）。**数据映射零变化**：分量下标恒=数据列下标
  （装配路径 `assembleSideVector` 不动），解析器零改动。
- **下拉弹出高度固定上限（20261113 修订二；B04 修订三补全）**：分量/类型
  （左栏视图下拉）/类型（文件页固定占位）/请求下拉统一 `setMaxVisibleItems(10)`
  （语义层保留；真机若生效与之取窄，行为无差）。**实测该语义在本项目 Qt 5.13.2
  运行环境不生效**（裸 QComboBox 设 maxVisibleItems=3 仍 27 行全展开零滚动，
  与 QSS 无关；用户实机 54 行全展开佐证，缺陷 B04），故以**确定性钳制**落地：
  四下拉 `view()` 装事件过滤器（`bindComboPopupHeight`），捕获弹出视图
  `QEvent::Show`——该时刻容器已按全部内容自然高完成布局（Qt show_helper 先
  激活布局再派发子件 Show，行高/余量实测有效），每次先复位 view 与弹出容器
  maxHeight 再判 `items>10` 才钳制（短列表自然全展不误钳），钳制值=首行
  sizeHint 行高×10+chrome 余量（视图框+容器差，不裁半行）；maximumSize 为硬
  约束，后续 resize 均被钳，跨环境（裸/QSS/真机）成立。实测（26 列矩阵文件）：
  弹出容器 600px→222px、垂直滚动条 range 18；6 项短列表 138px 自然高不变。
- **右栏最小宽度防线（20261112 二版）**：`_rightPanel` 设 minimumWidth 360
  （文件页行内容=标签 40+间距+浏览按钮/下拉+间距+路径框最小可用宽的实算
  下限取整，≥340 要求），窄宽下文件页控件（浏览按钮/路径框/类型/请求/分量）
  不再被裁成只剩行标签的"光杆标签"态；弹窗整体最小宽由布局
  自然推导（右栏最小宽+左栏 280+矢量/来源列+边距），QSizeGrip 在最小宽以上
  缩放照常、以下被 layout 挡住，DlgGeom 记忆尺寸小于最小宽时 Qt 自动钳制
  （默认 1000×560 与几何记忆不受影响）。
- **本期边界**：U/V 侧定义可编辑但不参与装配与校验（参数曲面定义后续接入）；
  编辑既有三维曲线（打开时载入图内曲线）未接；矩阵文件整表装配未接（任何
  类型均按所选列逐侧装配；整表网格口径用"从文件新建三维图"弹窗——网格类
  **真网格**已由数学网格模式（修订三二次拍板四类型）覆盖：瀑布/曲面/柱状
  Z 侧数学来源按 X×Y 全网格求值、曲线 Z 侧数学来源逐点配对条带，文件/数值
  来源仍为条带）；数学页 范围/拟合/滤波/曲线/
  函数 子对话框按键置灰；pch 结果文件来源未接（请求/分量固定"当前数据表"
  口径=所选 txt/csv）。
- **数学非有限值断口口径（R40，20261113；用户拍板方案 A"断口+提示"，极点
  钳制不做）**：数学求值遇 ±inf/NaN 统一按"断口"语义处理（对齐 2D 既有
  "NaN=断口"约定与 MATLAB/HyperGraph 行为），三层防线：
  ①**求值层清洗**（mCurveExpressionEvaluator，跨 2D/3D 共用）：evaluate/
  evaluateGrid 结果逐值 ±inf→NaN（数学域 NaN 保留），非有限点数经新增出参
  `int* nonFiniteCount`（默认 nullptr）带出——既有调用方零破坏，2D 侧仅
  清洗不加提示（`1/x` 类 inf 点按 NaN 断口渲染）；
  ②**引擎破洞**（Plot3D 独立仓 R40 提交 e7405f2/f4e2972）：五序列类
  setData 逐轴 minmax 扫描跳过非有限值（单点 ±inf 即拉爆归一化分母致整图
  无声消失的既有缺陷；全点非有限回落既有退化兜底 1e-4→max+=1 即 [0,1]）；
  曲面/瀑布含非有限顶点的 quad 整块跳过（破洞，不画半 quad 防跨洞拉面）、
  等高线单元任一顶点非有限跳过（±inf 会致 level 循环永真假死）、柱状非
  有限跳柱（实例序→数据序映射保拾取正确）、Line NaN 顶点 GL 裁剪天然断段
  （弧长累计跳过非有限点防虚线图案传染其后整段）；全非有限数据空 VBO 上传
  守卫（glBufferData(0,nullptr) 部分 GL 驱动段错误）；宿主
  `m3DChartView::unionAxisRange`（autoFit 范围+色带 z 范围单一实现）与
  `contourValueRange` 任一分量非有限的点不参与统计（含首点种子），全点非
  有限返回 false=查询失败走既有空数据路径——文件矩阵路径的 NaN 单元数据
  本身不改写，靠引擎破洞+并集防御（空单元格 kNaN 解析口径同样受益）；
  ③**统计提示**（本弹窗装配层）：网格模式经 evaluateGrid 计数、曲线逐点
  配对模式经 evaluatePointwiseXY 计数（修订三二次拍板）、条带模式
  （Z 侧数学来源）装配后直数 zs——0<M<N → 正常建图+输出信息
  "N/M 个网格点无法计算，已按断口显示" + **QMessageBox information 逐条目
  汇总明确告知**（形态拍板允许弹窗/状态行，取模态保证不被忽略）；
  M=N（全非法，如恒 NaN 表达式）→ 报错"全部 N 个网格点无法计算……未创建
  曲线"不建图（文档零写入，三维图都不建）；M=0 → 静默。回归：workspace
  `tools/r40test/tst_r40` 62 断言（求值清洗/统计出参/默认出参零破坏/网格
  与条带全非法拒绝/部分非法建图+模态提示/默认采样 ln(x)+ln(y) 201/10201
  验收场景/unionAxisRange 种子防御/像素级破洞+整面不消失+全 NaN 不崩溃）
  全绿；既有套件 define3dtest
  （117/65/113）/3dtest 全套/axis3dtest 75/r34test 83/ctest 基线一致
  （PostWindowTests/PostWindowIsolation 两失败系基线遗留）。
- **数值页交互对齐二维 + B03 闭合（20261113 修订五）**：数值页行操作整体
  对齐 2D mDefineCurveDlg 数值页（buildValuePage 同构）——表上方
  [+][-][清空] 工具条：[+]=图标钮 `操作/添加.svg` 24×24、tooltip"新增一行
  数值"、恒可用，点击表尾追加全 0 行（既有插入行初始值口径）并落新行 U 格
  直接可输；[-]=图标钮 `操作/删除.svg`、tooltip"删除选中行"、随选中行启用；
  [清空]=文字钮（ToolButtonTextBesideIcon，资源库无清空图标按 2D 同款文字
  占位）、tooltip"清空全部行"、随表行数启用，点击弹确认框「确认清空全部行
  数据？是否继续？」（取消/继续，默认继续——2D onClearValueRows 同款文案与
  语义）；启停联动收口 refreshValueToolBarStates 单点（itemSelectionChanged
  +整表回填尾部统一刷新）。表右键菜单六项（在上方插入一行/在下方插入一行/
  剪切/复制/粘贴/删除，图标与分组同 2D）：插入以右键行为基准上/下插全 0 行，
  剪切/复制=选中行进三维行剪贴板（4 值行结构原样沿用，不写系统剪贴板，与
  2D 1 值剪贴板不互通）且剪切删行，粘贴=剪贴板行插到最后一选中行之后（无
  选中=表尾），删除=选中行删；无右键行插入禁用、无选中剪切/复制/删除禁用、
  行剪贴板空粘贴禁用（2D 同款禁用口径）。右侧竖排五文字钮（剪切/复制/粘贴/
  插入/删除）**退役**（行为全由工具条+右键菜单承接，_vcutBtn 等五个头文件
  成员同步清理，连接随 UI 删除断开）。编辑触发加 AnyKeyPressed（键入即入
  编辑），选中行淡蓝 #D0E7FD 表级 QSS（::item:selected 文字 #303133+行头
  选中格规则，2D 1399-1405 同款）。清空后的空表为**持久态**：
  Define3DEntry::valueSeeded 首访种子标志只补一次 8 行零值，切条目/切侧回显
  不重新播种（2D 清空同口径；清空前删空行为一致化——删行不再自动回填零值
  行）。**B03 闭合**（缺陷单 20261112-B03，a2d6d426 存量）：用户编辑任意
  可编辑格即 cellChanged→onValueCellChanged→refreshValueCellStates→
  setFlags→dataChanged→cellChanged 无限递归栈溢出（0xC00000FD）——本仓
  Qt 5.13.2 实测 setFlags 同值重复调用仍发 dataChanged（setForeground 同值
  则正确早退），修复=refreshValueCellStates 内 setFlags 前同值守卫
  （`if (it->flags() != flags)`，缺陷单方向 1 主修复）+ onValueCellChanged
  末尾以局部 _syncing 包裹一次状态收口（缺陷单方向 2 双保险，护栏不泄漏到
  编辑入口判断之外）；修复后编辑合法值写条目 valueRows、非法值红字收口一次、
  递归链终止，应用装配含编辑值。回归：workspace tools/define3dtest 新增
  tst_define3dvalue 57 断言（B03 编辑不崩+写条目+红字收口、工具条/清空确认
  框/右键菜单六项、样式与触发对齐、空表持久不重播种）全绿；既有
  tst_define3ddlg 117/tst_define3dmatrix 113/tst_define3dgrid 110、r40test
  62、ctest 7/9（两失败系基线遗留）口径一致。
- **数值页来源切换转换链 + 非数值侧列回显 + 按侧脏确认（20261113 修订六，
  对齐 2D mDefineCurveDlg R21 同名链路）**：①**转换链**（`applySourceChange`，
  活动向量侧）：来源切到"数值"先把该侧**切出前**来源内容装配进该侧数值列
  （`dumpSideToValueText` 同名同语义，倒表须在该侧 source 落新值之前调用
  ——卫语句与装配分派才指向切出侧，2D R20-B02 同约束）——文件=已解析+已选
  分量列整列；数学=`assembleSideVector` 条带采样口径求值（X/U 侧 u∈[0,1]
  101 点，Y/Z 侧以装配的 X 数据为采样，与条带应用装配同源；**R39 网格求值
  不在此路径**，曲面 Z 侧转换同为单序列条带口径，含 y 表达式求值失败按失败
  口径清列不崩）；成功=该侧列逐行写 'f' 6、表行数扩到 max(现值Rows,数据
  长度)、扩行他列空串；旧来源无内容（未解析/未选分量/表达式空）=**保留已存
  数值列**（首访仍播种 8 行零值），有内容装配不出=**清空该侧列**（残留上次
  倒表内容会误导数值读数，2D 同口径）；离开"数值"不倒表（已存数值留条目）。
  ②**回显**：数值表整表重建时非数值侧列（X/Y/Z 列 source≠2）只读回显该侧
  当前来源装配数据（`computeSideEchoTokens` 'f' 6 token 现算缓存
  `_sideEchoTokens`，行位对齐、超出表行数留空、装配失败/无内容留空；表行数
  恒=valueRows 行数不按回显数据扩行——valueRows 是四列共享行结构的唯一
  存储，2D 按侧独立串存储才按各列最大值扩行）；行插删路径走
  `rebuildValueTableView` 复用缓存免重复装配，回显列按行位重排（token[r]
  位对齐）；U 列恒数值列身份恒可编辑不回显，V 侧无数值列转换链与回显均
  不适用。③**按侧脏标记**：Define3DEntry 增 x/y/z/uValueDirty 四侧标志——
  改格（数值侧列，U 列仅 U 侧来源=数值时计脏）置该侧脏；行级操作置脏全部
  数值侧（2D markValueRowOpDirty"置脏激活列"的 3D 适配——3D 全部数值侧列
  同为可编辑列；工具条 [+]/[-]/[清空] 与右键剪切/删除/粘贴置脏，右键插入
  一行/复制不置脏=2D 调用点口径）；程序回填（倒表/回显/播种/整表重建）
  不置脏；切入"数值"倒表后复位该侧脏标记（程序装配非用户改动，2D 同口径）。
  ④**脏确认**：离开"数值"（该侧切其他来源）且该侧脏时弹 2D
  leaveValueConfirmed 同文案确认框（「确认继续操作，刚才编辑的所有数据将会
  丢失。是否继续？」取消/继续，默认继续），取消=来源单选回退数值
  （QSignalBlocker 屏蔽 toggled 重入）状态零变化，确认=放行（该侧后续切回
  "数值"时按其旧来源重装配）。⑤**零变化红线**：onApply/assembleEntry/
  assembleSideVector 逐字节不动（数值来源装配现读 valueRows，转换后所见即
  所得）；向量侧切换不触发转换链仅回显该侧现状（3D 按侧模型与 2D R21 激活
  列模型差异，保持现状）；数学页界面零变化（表达式框+键盘原样，仅表达式
  内容参与转换链）；B03 修复双保险保持（refreshValueCellStates 扩展回显
  写入仍带 setFlags 同值守卫+显式同值 setText 守卫）。
- **Z 数学转换/回显二元按行求值 + U 列空白补 0（20261116 修订六补条）**：
  修订六转换/回显链对 Z 侧数学原走单变量条带采样（`assembleSideVector`），
  含 y 表达式（曲面 Z 侧二元，用户实例 `sin(x)*cos(y)`）求值失败→清列/回显
  留空；补条后 Z 侧数学（vector==kVecZ && source==1）转换与回显同改**逐点
  配对二元按行求值**（`zMathPointwiseEval` 弹窗私有辅助，转换/回显共用）：
  装配 X（数学来源=u∈[0,1] 101 点）→装配 Y（采样横坐标=装配出的 X）→按行位
  配对截齐到较少者（静默，转换/回显无 note 通道）→
  `mCurveExpressionEvaluator::evaluatePointwiseXY` 逐点 z_i=f(x_i,y_i)——与
  assembleEntry 曲线逐点模式同源序列（求值器零改动，调用方保证 xs/ys 等长）；
  单变量表达式在成对序列上为等价形态，统一走此路径。**网格类（瀑布/曲面/
  柱状）条目的 Z 侧数学回显为逐点配对口径**（修订六补条原文转换/回显同逐点；
  20261116 修订七起**网格类转换改整网格摊平**进表=数值网格态，见修订七①；
  回显维持逐点——R39 网格求值仅在应用装配路径，切回"数学"来源应用仍走
  assembleEntry 网格链路 evaluateGrid，零影响）。①**非有限值口径**：求值
  NaN/±inf 的行（R40 清洗后 NaN）
  数值列/回显 token 写**空串**（=装配跳行），不写 "nan"/"inf" 文本毒化应用；
  转换失败口径不变（真非法表达式清列不崩，旧来源无内容保留已存数值列）。
  ②**U 列空白补 0 三收口**（仅 U 补 0——X/Y/Z 空格不补，空=装配跳行的等长
  截齐语义依赖）：dumpSideToValueText 扩行新行 U 列写 "0"（其余扩行列仍空
  串）；rebuildValueTableView 整表重建时条目 U 空格归一写 "0"（写回
  valueRows+表格回填自然带出，表格不再出现 U 空白）；onValueCellChanged
  U 列收到空文本归一写 "0"（写回条目+表格同步显示，程序回写以局部 _syncing
  包裹防 cellChanged 重入）。③**应用装配链零变化**：onApply/assembleEntry/
  assembleSideVector/entryInMathGridMode 逐字节不动（修订六补条时点口径；
  20261116 修订七起 assembleEntry 增网格态前置分支、onApply 增绑定/原位更新
  收口，既有网格/逐点/条带三分流本体仍逐字节不动），网格/逐点/条带三分流与
  R39/R40 行为不回归（切到数值后应用=点列条带所见即所得，切回"数学"应用
  仍出网格；网格类切到数值后=整网格摊平+网格态直读装配，见修订七）。
- **数值网格态 + 应用原位更新（20261116 修订七，绑定语义对齐 2D
  editCurveId 口径）**：①**转换=整网格摊平**：网格类条目（瀑布/曲面/柱状）
  Z 侧数学切"数值"时 `dumpSideToValueText` 新增网格分支（先于曲线类逐点
  分支，条件 `vector==kVecZ && old==1 && entryInMathGridMode`）——镜像
  assembleEntry 网格模式装配序列（X 轴=u 采样/文件列、Y 轴=采样横坐标 X、
  `evaluateGrid` 全网格求值 zs/gridRows/gridCols），成功=清空现有 valueRows
  后逐单元写入 gridRows×gridCols 行（行主序：X[r*cols+c]=横轴第 c 值、
  Y=纵轴第 r 值、Z=单元值，'f' 6；NaN 单元写空串=断口；扩行即"新行"，U 列
  补 "0" 照旧），维度落条目 `valueGridRows/Cols`（`valueGrid=true`）——曲面
  从数值照常绘制的前提（表=整网格数据而非逐行配对点列）；装配不出（X/Y
  装配失败/表达式非法/全部网格点无法计算）=表原样保留+清 Z 列 false、网格态
  不落（既有"有内容装配不出"口径合流）。曲线类（plot3dType==1）维持逐点
  配对转换零变化；各类条目只读回显维持逐点口径。②**应用=网格直读装配**：
  `assembleEntry` 开头新增网格态前置分支（先于既有三分流，三分流本体逐字节
  不动；e 改非 const——降级清态须落条目）——网格类条目 `valueGrid=true`
  且 Z 侧来源=数值（类型判定独立进行，`entryInMathGridMode` 含 Z==数学条件
  网格态不满足）：表行数==维度时从表直读 X/Y/Z 三列（行=网格单元 row-major；
  X/Y 单元空格/非法=错误带行号，Z 单元空=NaN 断口与转换"NaN 单元写空串"
  对偶、非法文本=错误），rows/cols=落定维度入档（表即数据：不读各侧来源、
  不截齐——X/Y 侧自身来源定义暂不参与）；Z 单元非有限统计对齐 R40 网格口径
  （部分=断口+「N/M 个网格点无法计算，已按断口显示」note 复用模态提示、
  全部=报错不建图、零=静默）；表行数≠维度（增删行后）=清 valueGrid（维度
  只在转换时落定不回猜、降级不复原）+ note「网格行列与表行数不符，已按点列
  装配」走既有条带路径（诚实降级）。③**网格态表格**：网格态下 X/Y/Z 三列
  全部为可编辑数值列（`refreshValueCellStates`/`rebuildValueTableView` 按
  `entryInValueGridMode` 判定跳过"非数值侧只读回显"，`computeSideEchoTokens`
  网格态直接空缓存不为休眠侧来源白算）；U 列补 0 口径照旧；行插删按普通行
  处理（行数≠维度=应用时按②降级）；Z 侧切离"数值"=退网格态
  （`applySourceChange` 清 valueGrid，表内容保留原样不回滚，应用按各侧来源
  口径）；切回 Z 数值且数学来源有内容=重新倒表覆盖（新网格维度落定；无内容
  保留已存表=既有卫语句口径）。④**应用原位更新**：Define3DEntry 增
  `editCurveId`（2D 同名同语义，-1=新建条目）——应用时绑定节点存活
  （`nodeType==Curve` 查询，不做悬垂写入）=复用该 id 不 add3DCurve（条目名
  ≠节点名时 `setNodeProperty(kName)` 随宏可撤销）；无绑定/绑定失效（曲线
  被删）=`add3DCurve` 新建并回写绑定（重建重绑）；粘贴条目自动解除绑定
  （-1，2D 同口径；剪切=随条目删除）。宏结构：新建部分+名称命令包"定义
  曲线"宏（一次撤销整批还原新建与改名），数据 `set3DSeriesData` 直写宏外
  （既有先例：数据变更不进撤销栈，撤宏已直写数据留存——撤销语义边界文档
  明示）；成功提示区分新建/更新（纯新建维持既有文案，仅更新=「已更新 N 条」
  混合=「已定义 X 条、更新 Y 条」三维曲线）。既有三分流逐字节不动，
  2D 弹窗/求值器/引擎/控制器零改动（只调既有 API）。
- **打开载入既有曲线 + 应用溯源落模（20261116 修订八，编辑模式对齐 2D
  mDefineCurveDlg）**：①**打开即载入**：`showInstance` 每次打开先复位（未应用
  定义不落模型直接丢弃：条目/剪贴板/类型计数清零、列表清空），再载入目标三维
  图内全部 Curve 子节点为条目（`target3DChartId`=当前激活图表为三维图优先
  （存量多三维图会话跟随用户视口）、否则全文档首个同 onApply 落点口径；
  `addEntryFromCurve` 树序逐条，`editCurveId` 绑定随节点）；打开前选中曲线
  （或其 x/y/z 轴壳，回挂父曲线）为初始选中条目、类型视图下拉随之切换保证
  可见；无三维图/无曲线=初始空态（仅"新增/关闭"可用，同旧首开口径）。只落
  `_entries` 不碰列表 UI，批处理由 `loadExistingCurves` 收口重建过滤列表。
  ②**逐侧溯源还原**：数学表达式在（X/Y=`xRef/yRef.expression`、Z=新字段
  `z3DExpression`）=数学侧；文件分量列下标（新字段 `x/y/z3DComponent`）在且
  对解析缓存界内=文件侧（打开时即按路径解析）；缺席侧=数值兜底——当前数据倒
  数值列（`dumpCurveValuesToEntry`：三轴逐行 [U=0,X,Y,Z] 'f' 6、非有限写空串
  =装配跳行、仅兜底侧列写值其余列走只读回显；网格类条目持有效网格维度
  rows*cols==点数时落数值网格态 valueGridRows/Cols=plot3dRows/Cols，应用按
  维度直读装配渲染真网格；倒表后 valueSeeded=true 不再种子补行、按侧脏标记
  复位=程序倒表不算改动）；来源文件解析失败红字提示（带条目名）并整体数值
  兜底。③**应用溯源落模**：onApply 数据落档后逐条 `set3DCurveProvenance`
  （控制器新 API，`set3DSeriesData` 同款直写不进命令栈不发信号）——主标记
  数学>文件>数值；三侧表达式（X/Y 落 xRef/yRef.expression、Z 落 z3DExpression）
  与文件分量列下标按侧独立写入，非对应来源侧写空串/-1；pch 五元组/侧四元组
  清空（三维定义不使用，残留误导"更改曲线"等五元组消费方）。④**模型扩展**：
  `CurveSeriesData` 增 `z3DExpression`（Z 侧数学表达式）与 `x3DComponent/
  y3DComponent/z3DComponent`（文件分量列下标，-1=未记录/非文件侧）四字段；
  会话快照 **V13** 追写段（写侧 drawStyle 后恒追写、读侧版本门控——V12 及更早
  快照无此段保持缺省，旧 .mxses 读取零损伤兼容红线；分量列下标负值钳 -1；
  模板路径字段序冻结不含新字段，模板读入保持缺省）。既有"从文件新建三维图"
  等无溯源曲线=数值倒表态载入（仍可编辑，应用即原位更新+溯源落模，下次打开
  起完整还原）。

### 5.13.0 R23~R27 史料（部分已被 5.13.1 取代：独立卡片链路已退役）


曲线工作模式 Ribbon 页签最右新增**"3D曲线"**页签（主页/曲线/报告/3D曲线，
`mMenuWidget::_curveModeTabs`/`_curveMenuCandidates` 追加；PostWindow
`mPostMenuContributor::build3DCurvePage` 注册页面：文件组+"新建"大按钮）。
点"新建"弹"从文件新建三维图"弹窗，选数据文件与绘图类型（瀑布图/曲线/曲面/
柱状图）后经 **Basic/Plot3D**（Qt3DPlotWidget，QOpenGLWidget+原生 OpenGL
Core 3.3）生成 3D 图，主窗口在视口区开**独立卡片**展示（可旋转/缩放/激活/
关闭、可多卡并存）。**能力一句话**：从数据文件一键生成三维图并在独立视口卡片
中交互查看。

- **新增文件**（模块根）：
  - `mCreate3DPlotDlg.h/.cpp`："从文件新建三维图"弹窗（非模态单例白卡片，
    M徽标标题栏+✕可拖动，`DlgGeom::bind(this,"Create3DPlotDlg")` 几何记忆；
    数据文件行（只读回显过长省略+tooltip 全路径+浏览...，过滤器
    "数据文件 (*.txt *.csv)"）+ 绘图类型四单选（默认**曲线**）+ 提示行
    （默认灰字/错误红字带原因）+ 底部 取消/绘图（主蓝主钮）。绘图成功关窗
    （T2 起装配含无效单元按 0 兜底时卡片照建、提示行灰字带计数说明且暂不
    关窗），取消/✕ 直接关）；
  - `m3DPlotDataParser.h/.cpp`（T2 新增）：三维图数据纯解析器（纯静态无
    GUI，复用 R22 `mCurveAsciiReader` 完成字节→文本→切分/表头判定/编码，
    只复用零改动）——形态判定 `isMatrixTable`（表头行存在+表头 token 全
    "数值或空"+数据首列全有限+≥3 列）分**点列/矩阵表**两口径装配：
    点列 `assemblePointSeries`（前三列 X/Y/Z、2 列 Z=0 容错、非有限值行
    整行跳过带计数）供曲线/柱状；矩阵表 `assembleMatrixSeries`（横轴=表头
    数值 token 空首格跳过、纵轴=首列、Z 按 Plot3D Row-Major 展开为逐单元
    数组，行尾空字段=尾部全 NaN 列/行裁剪，内部非有限单元按 0 兜底带回
    计数，minZ/maxZ 供色带）供曲面/瀑布/柱状。无头单测
    `tests/PostCurve3D/tst_m3DPlotDataParser`（fixture 程序化生成；支持
    命令行传真实文件选配抽查）；
  - `m3DPlotViewportManager.h/.cpp`：3D 视口卡片请求中枢（小单例），信号
    `sig_request3DPlotViewport(QWidget* content, const QString& title)`——
    每成功绘图一次发一次，content=已完成 addItem+setData+fitToView 的
    Qt3DPlotWidget（所有权随信号移交主窗口卡片父链托管）。
- **主窗口接线**（mMechMainWindow/mViewBrowserWindow，QWidget* 级消费，
  不 include Plot3D 头）：连接信号 → `ViewportContainer::addViewport` 建卡
  （卡片 id 自 **`Plot3DViewportIdBase = 2000000`** 新基段起自增，与图表卡片
  `ChartViewportIdBase = 1000000` 并列；标题="<文件基名>-<类型名>"，重名允许）
  → 登记当前视图树页（布局由视图树唯一驱动，无页归属的卡片会在下次
  applyLayout 被隐藏，须落页才能常驻；modelID=-1 非渲染视口口径）→ 激活换入
  当前布局格（不切页/不切模式/树不动）。关闭：视图树右键"删除视口"对 3D 段
  分流走 `mMechMainWindow::closePlot3DViewport`（摘树节点+摘容器卡片；管理器
  原路径 deleteViewport 对未登记 id 静默无效）。
- **数据格式口径（T2 起宿主侧 m3DPlotDataParser 解析，Plot3D MapFileLoader
  仅作曲面/瀑布扁平网格兜底）**：
  **曲线/柱状图=点列**（逗号或空白分隔可混用、可有表头、跳空行；取前三列
  为 X/Y/Z，仅 2 列时 Z=0 容错不拒绝，非有限值行整行跳过；剩余 ≥1 行才
  成功）；**曲面/瀑布图=矩阵表优先**（首行=横轴值可空首格、首列=纵轴值、
  单元格=Z；行尾空字段容忍=尾部全 NaN 列/行先裁剪，内部非有限单元按 0
  兜底且提示行注明个数）+**扁平网格兜底**（矩阵表判定不成立或装配失败时
  回退 R23 `loadFile`：每行至少 4 列数值、同值 X 连续排列、z 取第 3 列，
  MapResult.txt 等 Plot3D 旧样例行为不回归）。**柱状图对矩阵表同样接受**
  （每格一根柱，柱宽按横/纵轴唯一值点距×0.85）；曲线选到矩阵表 → 红字
  "该文件为矩阵表，请选择曲面/瀑布图/柱状图"不建卡；两路都失败红字带原因。
  **3Dface 类两列对角线文件口径=忽略**（用户拍板 20261109，不针对性支持）；
  **RPM 瀑布专用格式（Plot3D 自带 Waterfall.txt）不支持**（非矩阵表口径、
  扁平网格兜底亦失败，红字提示即可，不接 loadWaterfallFile）。
  严格调用规范：**先 widget->addItem(series) 借 GL context 初始化，再
  series->setData**；换类型前 `scene()->clear()`（本弹窗一次绘图即一次建卡，
  未复用 widget）。色带/样式口径照 Plot3D DemoApp 加载分支（曲面 Cool/
  瀑布 Jet+RibbonWidth5、曲线亮橙 1.5、柱状 Rainbow，柱宽按点距×0.85、
  兜底 0.5——渲染端把数据归一化到 [-10,10] 盒子，宽度与数据原始尺度无关）。
- **校验与边界**：空选/文件不存在/解析失败（非有效点列、矩阵表与扁平网格
  两路均失败、矩阵表选曲线换类型提示）/异常 →
  提示行红字带原因，不建卡。**3D 图不入曲线树、不进 CurveDocument/命令栈、
  不持久化（会话内存活，.mxses 不含）**；单文件单图（一次"绘图"=一文件=一张
  图，多曲线文件不做拆分）；**OpenGL 3.3+ 硬件前提**（无硬件加速环境不可用）；
  样式不对齐参考软件（用户明确"大致相同即可"）；卡片激活走"离开曲线上下文"
  既有复位路径（id ≥ ChartViewportIdBase 的未知卡片反查图表为 -1），无需扩
  treeKeyForType/workModeForViewportType/ViewportType 枚举。
- **Plot3D 消费口径**：外部导入库（`Basic/Plot3D/install`，本仓库**不重建**）；
  `cmake/FindPlot3D.cmake`（头在 `install/include/opengl` 子目录，保持
  `#include <Qt3DPlotWidget.h>` 写法；Debug d 后缀区分）→ 根 CMakeLists
  `find_package(Plot3D REQUIRED)`（Qt 组件追加 `OpenGL`——QOpenGLWidget 属
  Qt5 的 QtOpenGL 模块）→ **仅 PostCurve 链接**（`_runtimes_libraries` 追加，
  同时进链接与运行时拷贝链，Plot3Dd.dll 随 InitRuntime/ctest PATH 覆盖）；
  Plot3D 头只在 PostCurve 模块内 include，PostWindow/MainWindow 经
  QWidget* 级信号接口消费，头面不出模块。

- **坐标系三面背景样式（20261109 R25，Plot3D dev 分支 96b5f91 起）**：坐标系
  背景面=按相机自适应的 3 个远端面（与网格线同一套选面逻辑），带不透明浅灰
  底填充（RGB≈0.94，写深度遮挡后方线框，沿面法向外偏盒尺寸 0.2% 防
  z-fighting），网格线浅灰（0.75 灰）；showGrid 开关关断语义不变（关某轴=
  该轴面填充与网格都不画）；数据系列盒内渲染恒在填充前方，任何视角不被遮挡。
- **建图轴量程随数据 + Line3D 归一化（20261109 T4，Plot3D dev 1b4013a 起）**：
  弹窗每类成功 setData 后用与传给 setData 完全相同的 float 数组现算各轴
  [min,max]，并逐类型复刻对应序列 setData 的退化兜底口径（Line3D/Surface/
  Bar 为 |max-min|<1e-4 → max+=1；Waterfall 的 setData 无 +1 兜底，取原始
  min/max），调 `plot->scene()->axis()->setRange` 定点设置——**不走
  updateSettings**（其整包覆盖 type/colormap/lockInteraction 等默认值），
  时序在 setData 之后、fitToView 之前，刻度 overlay 下帧现读即生效。**建图
  后轴量程=所绘数据有效范围，刻度数字即真实数据坐标**（不再恒为默认
  X/Y ±10、Z 0..10）。同时 Line3D 自该版本起按序列约定补齐数据范围归一化：
  渲染与拾取两套顶点着色器均经 u_DataMin/u_DataMax 映射进坐标盒（与
  Surface3DSeries 同构，拾取像素与可见像素一致），并如 Surface 在 init
  时机兜底重建几何——修复"setData 早于 widget 首显（无 GL 上下文上传落空）
  导致曲线永不绘制"的失配；归一化后曲线充满坐标盒（端点触壁属预期）。
- **坐标系三面固定朝向 + 纯背景语义（20261110 R27，Plot3D dev 7185309 起）**：
  三背景面不再随相机翻转——Axis3D 的墙面绘制/刻度贴边/轴标题定位三处逻辑
  统一固定为默认相机视角（reset 方位 az=-25.5°/el=30°，eye 位于
  (-X,+Y,+Z) 象限）下的三个远端面 **X_Max/Y_Min/Z_Min**（常量
  `kFixedBackdropFace` 集中一处可改），旋转全程墙面/网格/刻度/轴标题零翻转；
  填充与网格改纯背景绘制（先于数据、`glDepthMask(GL_FALSE)` 不写深度，
  深度写开关/背面剔除进出配对恢复），任何视角（含墙的几何背面）不遮挡数据，
  默认视角观感与 R25 一致；showGrid 开关关断语义不变。

## 5.14 极坐标图渲染 chartType=3 + 柱状图补强（add-modal-contribution-analysis，本轮已实现）

### 极坐标画布（自绘图元族，旁路直角轴带）

- **落型与布局**：`kChartType=3` 命令→全量 rebuild；图表内 X1/Y1 直角轴带隐藏，
  自管两族图元：`mCurvePolarFrameItem`（Z5 底图：同心圆网格+放射线+角度刻度，
  均匀/固定步长默认 45°、刻度总数直接生效；半径刻度复用 mCurveAxisTickEngine
  缩放序列映射 [0,R]，归一收口 applyPolarDecor 一处）+ `mCurvePolarSeriesItem`
  （Z20 数据曲线）。半径轴标题沿 0° 方向外侧绘制。
- **半径缩放域**：Linear [0,max] / Log [-3,1] / dB [-30,10]（dB20 参考值默认
  1.0、每十倍刻度数/网格线数默认 5/11），手动档取 Y1 min/max；刻度经
  mCurveUniformTickDiv/mCurveDecadeTickDiv 在变换空间生成，计权/参考值读 Y1
  轴节点口径（与直角轴同链）。dB10/dB20/Log/Linear 均生效，计权先加权后变换。
- **两形态**（"显示"行联动）：散开（各向量自极点射线+±25° 后掠箭须，零幅值
  退化端点圆点）与首尾相接（向量链）；链形态=视图态 `setPolarChainForm`
  （不进命令栈，rebuild 重放；面板通道 applyPolarChainForm 随产出物化后补发，
  读 NVHDisplayOptions.displayPhase）。极坐标仅 Linear 半径档（design 69），
  显示选项对话框其余四档置灰、存量非 Linear 回显并回写 Linear。
- **交互边界**：画布点击命中走 hitPolarSeries 独立通道，频率拾取广播在极坐标
  下抑制；引擎无框选缩放器天然禁框选；fit 经 fitChartAxes 极坐标早退固定
  [-1.05,1.05]；页眉页脚/图例属性通道 chartType 无关直接生效。
- **属性页联动**：图表内 X1 类型回显"相位"/Y1 回显"恒值"（只读覆写，Y1 文本=
  轴标题）；缩放方式非 Linear 隐藏"刻度生成方式"行、显示"每十倍刻度数/每十倍
  网格线数"（dB 档另有参考值+计权行），Linear 反之（polarAxisModeOf 助手）。
- **树命名**：每模态复数三壳 f/ym/yp，显示名覆写空格分隔 "<幅值> : Mode N :
  <频率> Hz"（"<幅值> : Response"/"<幅值> : Others" 同格式）。
- **消费方**：NVH 工具集面板（PostWindow::mPostNVHContributionPanel，
  producePolarChart 产出模态向量族）。

### 柱状图补强（同变更 §3.5/3.6）

- **单柱曲线**：引擎口径一侧一曲线多柱，类目签挂曲线（`setSeriesTickLabels`）；
  M+2 柱=Response+前 N+Others。
- **柱顶数值标注**：`mCurveBarLabelItem` 自管图元（随柱状增量刷新重建——"data"
  属性变更本就落 scheduleRebuild 全量路径）；文本 'g'6、正上负下、Z=30。
- **投影组合附加物**：4 条 Datum 红基准线（addBaseline，上下两条与 Y 轴极值
  重合随缩放联动，中间两条 ±半幅占位待参考软件定标）+ Note 贡献列表粉框注释
  （行 "<±值> : <名称>[ : <频率 Hz>][ : [<%>]]"，值恒带 +/− 号，占比段仅
  %Plot 两位小数，末行 Ranked By 英文名）。
- **%Plot 数据级换算**：y=贡献/总响应×100、轴 Linear、Y 轴 "% Participation"
  （design D6）。
## 5.15 批量曲线数据落地门面 addCurveBatch（2026-11-12，首个模块外修改类调用）

模块此前所有"建页/图/曲线/灌数据"调用方（NTF/VTF/动刚度/从文件创建/定义曲线）
都在模块内各自复制同一套 `beginMacro → addPage/addChart → addCurve →
setSeriesData/setSeriesSource → 配色 → endMacro → activate` 样板。后处理曲线
提取（PostWindow::mPostCurveExtractPanel）成为**第一个模块外的修改类调用方**，
按门面收口哲学新增高层批量接口，样板全部收进模块内：

```cpp
//CurveDefine.h（数据契约）
struct CurveBatchItem {          //单条曲线：name/xData/yData/group/来源五元组
    QString name, group;         //group=分图分组键（OnePerGroup 布局；空=该条独立一图）
    QString sourceFile, subcase, schema, object, component;
};
enum class CurveBatchLayout {    //枚举序 0-4 与曲线提取面板绘制选项一一对应
    AllInOne, OnePerGroup, OnePerCurve, NewPage, ExistingChart
};
//mCurveDocController（门面）
int addCurveBatch(items, layout, pageName, existingChartId, xTitle, yTitle, currentChartId);
```

行为口径：
- **静默落图**：宏期间控制器发 `sig_requestDeferCardActivation(true/false)`，视口工厂
  构造时连接响应 `setDeferCardActivation`——卡片只登记不切页/不激活/不物化，用户
  当前视图页不动，双击树节点/点卡片时由模块既有懒物化机制渲染（后处理曲线提取
  的用户定案："绘制不切换视图，静默绘制"）。不调 activatePageFirstChart/
  activateChartViewport。**控制层经信号协调视图层而非直接引用**——CurveCore 保持
  无 CurveView 依赖（tst_mCurveDocName 无头单测直接编译模型层，不链视图链）。
- **整组撤销**：宏包裹；全部曲线失败时 `_undoStack->undo()` 整宏回滚不留空页。
- **布局分支**：AllInOne 落 currentChartId 所属页（调用方查工厂当前激活图表传入，
  无效/未传则新建页）；NewPage/OnePerGroup/OnePerCurve 新建页；ExistingChart 校验后
  addCurve 并入（配色从既有曲线数续起防撞色，mOpenCurveDlg 口径；只追加曲线不
  覆盖轴标题）。页内多图按 curveViewportLayoutSlots 取最小容纳格数设 kLayout，
  超上限自动分页。
- **每条曲线**：addCurve→setSeriesData→setSeriesSource→kColor（色轮轮换）→
  kLineWidth(2.0)；空数据点跳过并 reportWarning；xTitle/yTitle 非空设各新建图表
  X1/Y1 轴标题。
- 返回落地页 id（ExistingChart 返回并入图表所属页）；items 空/目标失效/全部
  失败返回 -1。

踩坑留档：局部变量禁命名 `slots`——Qt 关键字宏（`#define slots`，未定义
QT_NO_KEYWORDS 的目标里被替换为空直接语法崩塌；本次 tst 目标报 C2143 而 DLL
目标恰好编过，属目标间预处理差异，勿再踩）。

首个消费方：后处理曲线提取面板"绘制"（readTimeVarCurve 提取全帧时间历程后
一次调用成图）。后续其他模块（查询面板、表格数据等）"把数据画成曲线"复用
同一接口，不再各自复制样板。

## 6. 后续功能挂载点

| 功能（设计图） | 挂载方式 |
|---|---|
| 定义曲线弹窗（10~13） | **已重构完成**（mDefineCurveDlg，见 5.5）：参考软件形态——曲线列表+剪切/复制/粘贴/新增状态机（图1初始置灰→图2新增可用）、向量X/Y+来源 文件/数学运算/数值 三页切换（图3 表达式构建器+曲线选择器 fx+计算求解+预览）；数学运算经 mCurveExpressionEvaluator（自研引擎）求值，表达式持久化到 x/y 壳节点 kExpression |
| 更改曲线（13） | **已实现**：x/y 数据页"更改…"（两侧同源，换整条曲线）=文件对话框选数据文件（R22 起过滤器含 csv/txt；**不弹选择器**——pickChannel 弹出形态自 49697f80 起退出交互，控件能力保留现无调用点，页面通道改选用页内自有四级联动下拉（mCurveDataPropPage 级联实现，与选择器同口径的独立实现，R22 起同样 ascii 三级化））→ 导入并落默认通道（pch=首个可用矢量通道；csv/txt=[Unknown]/[Block 1]/第 1 列，R22；数学运算/数值等非文件来源失配时同样落文件默认通道）→ Store `assembleChannelData` 装配（规则/防线与两弹窗同源；R22 起按 fileKind 分流，ascii 分量=列名首个匹配、xData=行号（装配 API 层；整线换源实际 X 落第 1 列通道形态，不用行号））→ `mEditCurveDlg::snapshotCurve` 压撤回快照 → `setSeriesSource`+`setSeriesData`+清空 hiddenRanges（旧区间指向旧数据坐标）**数据直写不进命令栈**（与 pch 导入同语义，Ctrl+Z 不含换源，撤回去编辑曲线"撤回"按钮）；页面回显因来源直写静默+数据信号发父曲线 id（页绑壳节点）而由 changeSource 手动回填；复数曲线换 csv/txt 拒绝（无复数分量对，不触发 siblingComplexComponent 成对换源） |
| 基准线（14） | **已通**：Ribbon"基准线"下拉（纵向/横向互斥、与注释模式互斥）-> 工厂广播 -> 模式内画布左键连续创建（吸附点优先，一次撤销宏）、悬停曲线 X/Y 读数（mCurveReadoutItem）；增量渲染保缩放；双击/右键"编辑"弹基准线编辑器页（mBaselineEditorPropPage，图4，**行集与参数栏页一致**）/删除；参数栏页对齐参考软件（见 5.1 表 09 行），**出厂默认线色蓝、实线**；**线上标签已渲染**（mCurveBaselineLabel 自绘图元，CurveView：标签非空即画在线上，创建时初值=基准线名称"Datum N"；显示组字体设置=标签字体（族名空=默认）、标签位置 9 宫格=沿线/贴边定位（纵线：上下沿画布、左右分线两侧；横线对称），文字色随线色，线越界不画）；仍缺：拖拽移动 |
| 注释（15） | 渲染/选中/右键菜单/属性编辑器已通；显示组（对齐/边框颜色/自动定位）模型属性已入（渲染接线下一期）；仍缺：依赖表达式求值、手柄拖拽调框/拖动移动 |
| 编辑曲线（16） | **会话式编辑已实现**：Ribbon checkable 按钮 -> 视口工厂编辑会话（锁定激活视窗 + **目标曲线集**（默认首条；`sig_editCurveStarted(chartId)`/`sig_editCurveTargetsChanged()` 广播，范围/方向/步长状态广播，数据界取目标集并集））-> 目标视窗裁剪线拖拽/Ctrl+点选换目标（单条替换目标集）/全曲线 20% 压暗 + 每条目标截取段旁路不透明重绘（`_editSegments` 按曲线id 的 QHash 旁路图元，随目标集增删同步；mCurveTrimMarker 旁路图元），浮动窗口 mEditCurveDlg（目标曲线/方向/起始/终止/步长 + 底部四按钮；样式对齐设计图16：多行曲线名回显框（多条目标以"，"连接）+框内右下角波浪线选择按钮、虚线描边等宽步进按钮、等宽加高底部按钮、起始/终止 4 位小数——`mCurveFoldSection` 新增可选 `setHeaderHeight`，默认 25px 不变不影响参数栏）。选择按钮弹**曲线选取器**（480×500 无框白卡片：M徽标标题栏可拖动 + 全选/全取消/反选工具条 + 工程->页->图表->曲线层级树 + 取消/确定；树条目多选（ExtendedSelection），图标复用导出的 `curveNodeStatusIcon` 绿√合成；打开时预选当前目标集；其他视窗曲线置灰）；**确定把整组选中设为编辑目标**（工厂 `setEditTargetCurves`，本图表内；跨视窗编辑仍为预留扩展）：**裁剪**=仅保留区间内数据（范围外落盘删除）；**删除**=删除区间内数据点（与裁剪互逆）；**隐藏**=区间内数据段隐藏为断口（模型 `CurveSeriesData.hiddenRanges` [轴,lo,hi] 三元组存原始空间，渲染 `transformedSeries` 对隐藏点 NaN 原位占位——经离线像素验证 Qwt/Qt 折线遇 NaN 断线；`fitChartAxes` 自动范围同步跳过隐藏点）。三者对**每条目标曲线独立执行**（逐条判断无点/将清空则跳过该条，全部无效才提示未执行），均 setSeriesData/setSeriesHiddenRanges **数据直写不进命令栈**（与 pch 导入一致），执行前把当时全部目标曲线状态压入**操作组快照栈**（一次操作一组、一次撤回整组恢复；快照含来源五元组，栈经公开 `snapshotCurve` 同供参数栏"更改…"换源压栈，撤回时数据与来源一并还原；弹窗单例自管，跨编辑会话保留；深度上限 50 组）：**撤回**连续点击逐级退回未编辑状态，栈空/无历史时按钮置灰。仍缺：跨视窗同时编辑多曲线。后续拖点编辑再按工具状态机演进 |
| 多窗口同步（09） | **应用样式已实现**（mApplyStyleDlg，见 5.4）；仍缺：视口编组广播游标/缩放（SyncGroupController 观察者） |
| 第二坐标轴（07） | **已升级为多坐标轴功能**（见 5.6）：XAxis2/YAxis2 均可多枚，轴区右键"添加坐标轴"叠加 X2/X3.../Y2/Y3...，首枚占引擎上/右轴（渲染已启用）、其余自绘条带；曲线可挂首枚副轴（yRight/xTop 已接线） |
| 属性页补全（03.x~08） | 参数栏 12 类面板已按设计图补齐（见 5.1）；弹窗专属页已有曲线/坐标轴/图例/图表/注释/基准线，其余类型回退参数栏页（迁移期策略） |
| 曲线标签渲染（03.1） | **已实现（图例文本方案）**：勾选"显示标签"的曲线，图例条目显示解析后的标签文本——前缀/内容/后缀各受开关，解析为空跳过、"-"连接（同绘图选项标题效果）；变量取值同 x/y 数据页侧来源回显约定，{max(y)}/{x} 取 Y 最大值及其 X（跳过非有限/隐藏点）；未勾选显示曲线名；复数图取上面板侧标签。详见 5.1 渲染清单"曲线标签渲染"段落 |
| 页面布局渲染（01，视口平铺） | **kLayout 首消费方=报告导出侧（20261102-R07 已实现）**：`PostCurveGlobal.h` 新增 `curvePageLayoutGrid(kLayout)` 派生表——20 预设逐个给出等效规则网格行列（与 `curveViewportIconPaths`/主窗口 viewLayoutTable 同序同源收口，已与各 svg 图标逐个核对；索引 3~6 不规则预设退化 2×2 并逐个注明），kLayout≤0（默认未选）/越界返回 {0,0}=自动。报告导出按所选布局放置/拼合该页图窗截图（勾选态透传节级 gridCols/gridRows 给三渲染器，不勾选态 `composePageImage` 按列数拼合）。**20261102-R08 拉伸充满**：导出图充满 PPT 内容插入框（勾选态格子=插入框按行列均分、图非等比拉伸入格；不勾选态 `composePageImage` 画布固定为插入框宽高比 5670×2860（不再随图窗卡片几何变化），整图拉伸入框；单图页独占整框；Word 行高撑满版心、HTML 宫格同框比例 object-fit:fill，详见 doc/report-export-architecture-design.md §8.7）。R06/R07 等比居中时代的"逐字节一致"回归基线随之作废（新基线=R08 几何断言，workspace tools/r10_harness），3D 老链路（无 templateMode 键）字节回归红线保留。**视口平铺渲染仍是下一期**——本消费仅在导出侧，画布视口分布不随 kLayout 变化。**20261103-R09 导出目检修正**：docx 导出清空封面设计参数表右列值（左列标签保留供用户自填，自定义模板无该表静默跳过），HTML 模板分支定位栏/顶栏 CSS 对齐两张样张像素量取值（侧栏 225px/条目 14px·行高 24px/分隔线 2px #303133/顶栏 60px，锚点跳转与 R08 宫格零触碰，workspace tools/r11_harness），详见 doc/report-export-architecture-design.md §8.8。**20261103-R11 定位栏加大+滚动定位**：HTML 侧栏 300px/条目 15px·行高 32px/sticky 固定可见（height:100vh）/自身可滚动（overflow-y:auto 细滚动条）+ scrollspy 滚动高亮（文末内联原生 JS，按 section offsetTop 判定当前页切换 .active：主题蓝·加粗·左 3px 指示条，点击锚点/resize 联动）——**有意偏离 R09 样张量取基线**（用户实机体验优先；logo 格随侧栏 300px 同宽同线（评审跟进项），顶栏其余数值维持量取值），r11_harness 断言随基线更新，详见 doc/report-export-architecture-design.md §8.8。**20261103-R13 PPT 内容页克隆**：PPT 模板分支内容页从"删除全部模板页后内置几何白纸重画"升级为**克隆模板自己的内容页样例页**（sldIdLst 序页2=勾选态/页3=不勾选态，只其一则两态共用，都缺回退内置几何）——标题占位符填页名、图片占位符 extent 作插入框（无则内置 TPL_PAGEIMG_XYWH 兜底）、普通示例图与插入框相交删除/不相交装饰保留、其余形状原样，用户自定义内容页装饰（艺术字/文本框等）不再丢失；默认母版同走克隆（几何与内置路径同值，仅 XML 来源变化——R08"内置几何"XML 字节基线作废，新基线=克隆产物，几何断言保留为回归线，workspace tools/r14_harness），**20261104-R15 兜底框随画布缩放+标题规范化**：PPT 兜底插入框 TPL_PAGEIMG_XYWH 与内置标题带 TPL_TITLE_XYWH 由母版画布绝对 EMU 改为按模板 sldSz 逐轴等比缩放（母版画布恒等=默认母版零回归；图片占位符派生框不缩放），标题与插入框相交时由 B05"重定位框上方保留原字号"改为规范化为缩放版标准标题带+页名 run 显式 20pt（置顶/timing 语义沿用），**20261104-B06 规范化补段落对齐**：页名 run 所在段 pPr 显式 algn="l"（封面版式 lstStyle/母版 titleStyle 继承居中纠正——几何对而文字带内居中；锚定页名段只动该段，pPr 缺失构造插为 a:p 首子元素、带子元素只动属性；不相交标题零变化），自定义模板（4:3/16:9）内容图充满版心、标题左上标准带左对齐，详见 doc/report-export-architecture-design.md §8.4/§8.10 |

**新增弹窗统一接几何持久化**：构造函数 `resize(默认尺寸)` 之后加一行 `DlgGeom::bind(this, "DlgKey")`（无边框/落点由调用方指定的弹窗用 `DlgGeom::Mode::SizeOnly` 只记宽高），即完成"恢复上次大小 + 隐藏自动保存"，无需其他代码；键名稳定（类改名不丢用户已存几何），详见 mDlgGeometry.h 头注释。当前已接入：mOpenCurveDlg / mDrawingOptions / mPropertyEditorPopup / mApplyStyleDlg / mDefineCurveDlg。

## 7. 已知限制（渲染对齐自绘原型后）

已对齐项（与 NVHCurveWgt 原型效果一致）：
- 刻度模式 Linear/Logarithmic/dB20/dB10（**数据预变换+线性轴**：模型存原始值，
  渲染层按轴 scaleMode 现算，刻度数字显示变换空间的值；非法点≤0 过滤断线）
- 网格常开（浅灰1px）、画布外框（黑2px）、零线十字（双Linear时 x=0/y=0）
- 刻度数字 'g'4 位有效数字（自定义 QwtScaleDraw）
- 页眉：文本=首条曲线名 + 字体 + 左/中/右对齐（引擎 PlotView 页眉条带）
- 页脚：同页眉（引擎 PlotView 页脚条带，applyFooter 与页眉对称接入）
- 轴标题字体、刻度字体（AxisSettings titleFont/labelFont）
- 自动范围：变换空间取 min/max，X±5% / Y±10%（复刻原型 fitAll）
- 线宽/线型方案、符号形状（10种：无+9符号；五角星为 PlotView 扩展样式 Sym_Star5 经 mPlotSymbol 渲染，米字 Star1 库内保留，其余映射 QwtSymbol；实际大小=基准(大圆点12/其余8)×缩放尺寸，密度=间隔）、图例粗体、六色轮换

仍存在的差异（接受/待做）：
- 曲线右键菜单的剪切/复制/粘贴置灰占位（模块尚无曲线剪贴板，与 Ribbon"编辑"组同策略）
- 外观/符号/标签/标题等值空间不变的变更已不丢用户缩放：符号与线条外观走增量刷新（scheduleSeriesStyleRefresh，5.7），其余全量重建但恢复"用户缩放过"轴的分隔（5.7）；数据形态键（刻度模式/计权/挂轴/图表类型/换源）仍按新数据重新适配
- 页眉/页脚文字颜色已接线（kHeaderColor/kFooterColor，applyHeader/applyFooter
  经引擎 PageBandLine::color 逐行下发；选中态主题蓝+加粗、未选中=模型色/无效
  兜底黑；引擎配套改动：mPlotView headerWidget()/footerWidget() 条带控件访问器
  + PageBandLine 颜色字段，Basic/PlotView dev 分支 71dfb7d 及以后）
- 图例边框样式/颜色、曲线文本色、引线位置、自动定位、自定义位置均已渲染（引擎 mPlotLegendItem setter 扩展）；图外四向位置待引擎外置图例（见渲染接线清单）
- 固定像素边距（原型 L120/R5/T80/B110）：保留 Qwt 自适应留位（按百分比硬算会随窗口尺寸漂移）
- 加权/计权（A/B/C/U）**已实现于渲染层**（CurveAxisData.weightMode，kWeightMode 走命令栈）：
  统一入口 `mCurveChartView::transformPoint`（计权线性增益 10^(修正dB/20) -> 刻度变换，
  对齐原型"先加权后变换"顺序；X 计权以自身值为频率，Y 计权以同索引 X 原始值为频率），
  全刻度模式生效（dB20 下即标准声学语义 "dB值+修正dB"）；曲线/自动范围/编辑截取/
  工厂界值四处逐点换算同源，游标/基准线/读数等渲染空间显示自动一致。
  U=unweighted 与"无"等效（保留选项对齐绘图选项下拉回显）。A/B/C 为 IEC 60651/61672
  标准公式（mCurveWeighting.h，f<=0 不修正）。边界：手动轴范围/编辑截取落盘等标量
  换算无频率上下文，仅刻度变换不加权（applyAxis/displayedAxisRange/条带/mEditCurveDlg
  四处已注释）。绘图选项加权经 mOpenCurveDlg 映射 kWeightMode；副轴创建复制、应用样式
  （mark.scale 组）均携带计权
- 注释已渲染（库图元 PlotView::mPlotAnnotation：边框文本框；曲线注释带引出线且
  边框/文本/引出线用所附曲线色，无锚点符号）：Ribbon"注释"按钮进入注释模式，
  左键打点创建（吸附走引擎 mPlotSnapper 仅吸附模式，半径 30px，吸附信号驱动
  预览与创建锚点，采样点语义不变）、悬停曲线实时预览、退出后双击注释边框内部弹出
  mPostAnnotationDialog 编辑模板文本（{X}/{Y}/{Time}/{Label}/{Slope}/{Curv} 求值显示，
  "视图注释"占位回填默认模板 Y={Y} @ X={X}）；曲线外点击先置"视图注释"占位；
  注释的创建/属性变化/删除走增量刷新，不影响用户缩放状态。
  选中与属性编辑（设计图15，已实现）：单击注释选中（边框外主题蓝选中框——
  与曲线选中高亮同色 curveHighlightBlue，四角/四边中点手柄，经选择中枢与曲线树
  双向联动，点空白清除）；右键注释弹菜单
  （编辑/复制置灰占位/删除走命令栈/隐藏置灰占位）；"编辑"经 sig_requestEditorPopup
  弹出注释属性编辑器（弹窗专属页 mAnnotationEditorPropPage：标注[名称/文本/依赖
  表达式(依赖于/依赖曲线/相位点/位置)]+显示[最小化/字体背景/对齐方式/边框颜色/
  文本颜色/自动定位(锚点/偏移)]；名称/文本/位置/文本颜色接既有模型属性即时生效，
  其余为设计占位，条目超弹窗高度时整页滚动）
- mOpenCurveDlg 的 sig_requestNewViewport 信号已随框架收编删除（视口请求唯一通道为视口工厂）
- 曲线统计（5.12）仅识别生成功能打标后的曲线：升级前生成的旧曲线无
  CurveCategory 标记、统计不到，属预期不做事后兼容，重新生成即可
- 工程为 file(GLOB_RECURSE) 收源，新增文件后需重跑 CMake（VS 的 ZERO_CHECK 会自动触发）

## 8. 视图架构差异（dev_base 合并映射说明，2026-09-28）

本模块自 dev_base 工作目录快照合并而来。dev_base 侧的图表视口建立在旧 MDI 架构
（QMdiSubWindow / mViewportModelManager::getPreCurrentWindow->setWidget）上；
htx_mech_treeMenu 分支已切换到新视图架构（图表 view 作为业务内容部件挂进
ViewportContainer 的卡片，页归属由视图树驱动）。合并按下表映射，功能语义保持不变：

| dev_base（MDI） | 合并后（视口卡片） |
|---|---|
| `sig_requestNewViewport(type,name)` + `getPreCurrentWindow()->setWidget` | `sig_requestChartViewport(view, 名, curvePageId, viewPageId)`（主窗口建卡片并挂视图页） |
| `onViewportActivated(QMdiSubWindow*)` 反查 `chartIdForSubWindow` | 主窗口监听容器 `sig_activeViewportChanged` 后回投 `notifyChartViewportActivated(view*)`（含非曲线视口的 -1 广播与树上选中联动） |
| `currentChartId()` 恒 -1（旧通道已废） | `notifyChartViewportActivated` 维护 `_currentChartId`；定义/编辑曲线的开启目标经此取 |
| `refreshChartViewTitles()` 直改 QMdiSubWindow 标题 | 经 `sig_requestRetitleChartViewport` 请求主窗口改卡片标题；独立窗口兜底时直接 setWindowTitle |
| `placeInCurrentViewport` "空视口放入" | 恒 false 降级为新建卡片（新架构视口创建即带渲染内容，无空视口概念） |
| MDI 标题 = nodeFullDisplayName "p1w1 : XY Plot" | 卡片标题同源；宿主视图树登记名创建后不回写（同 MDI 时期语义） |

其余差异：
- pch 导入页命名走"页对齐"（选已有视图页同名 / 新建视图页后由主窗口回填页名），
  dev_base 的"文件基名命名"被其取代；addPage 空名缺省名仍为 Page 1...（HyperGraph 风格）。
- 曲线 Ribbon 入口在 PostWindow 的 mPostMenuContributor::buildNVHCurvePage
  （定义曲线/编辑曲线/基准线/注释按钮及工厂信号接线），非 mMenuWidget。
- PlotView 引擎需同步至含 CurveSettings::legendText / mPlotLegendWidget 的版本
  （Basic/PlotView dev 分支 f5f0ff9b 及以后），否则本模块编译不过。

## 9. dev_base 增量合并（2026-09-29，快照后新提交）

在 2026-09-28 快照导入（对应 dev_base 提交 49697f80）基础上，追加合并 dev_base
PostCurve 的两个新提交：4e424390（样式更改以及部分功能实现）与 eb6cf057
（样式部分更改）。18 个文件，冲突 1 处。主要内容：

- X/Y 数据属性"缩放系数/偏移量"接入渲染管线（CurveDataRefData::applyTo 预变换，
  作用于渲染/自动范围/编辑裁剪/工厂界值/标签统计，模型仍存原始值）；负缩放语义
  对齐参考软件（-n = 缩小为 1/n，不翻转不产生负值）。
- 曲线标签默认值调整（默认显示标签、内容模板 {y.MXRequest}-{y.MXComponent}、
  前后缀默认不显示）；图例文本支持"取消显示标签后文字消失不回退曲线名"
  （引擎新增 CurveSettings::legendFallbackToName）。
- 用户平移/缩放期间自绘副轴条带实时跟随（监听引擎 signalViewChanged 轻量刷
  mCurveAxisStripWidget::refreshScale，严禁走 rebuild）。
- 属性编辑器弹窗宽度锁 280、移除副标题行（host() 去掉 title 参数）。
- 大批 UI 样式统一：弹窗主钮统一主蓝 #1385F6 / 44x26、图标按钮 24x24 切资源切图
  （:/mMainWidgets/Icon/Curve/操作/打开文件.svg）、"从文件创建曲线"白底改版、
  列间隔 16px、右键菜单固定宽 144、绘图选项弹窗刷白等。

冲突处理：mCurvePropPages.cpp 的 pageLayout 边距——dev_base 改为 (10,6,10,6)，
本侧维持 bfead51a 的"不设边距、用布局默认值"（三方共用参数栏骨架规格），按本侧解决。

引擎配套（Basic/PlotView，dev 分支 5f0ff9b → 97679fe，4 文件同步到工作区未提交）：
- 22e00e1 图例文本部分修改（可以留空）：CurveSettings::legendFallbackToName +
  图例条目显示文本回退逻辑（本模块编译必需）。
- 1311f12 移动时实时生成：接管 QwtPanner 手势逐 move 改轴范围并立即 replot、
  画布 ImmediatePaint 同步重绘、倾斜轮/平移期间滚轮忽略（副轴条带实时跟随的
  引擎侧前提，signalViewChanged 手势期间逐步广播）。

## 10. 模板曲线与曲线图层（add-template-curve，本轮已实现）

把当前曲线会话（页/图表/曲线/基准线/注释及其样式）存为 `.mxtpl` 模板，后续对
任意会话（可换数据文件）重新应用；一次应用产生一个"图层"，可整体显隐/删除。

### 模板格式与序列化（CurveCore）

- `CurveDocSerialize::saveTemplateSubtree/restoreTemplateSubtree`：模板页子树
  变体——写结构+类型化属性，**置空 xData/yData/complexYData**（模板不内嵌数据）；
  文件来源曲线的 sourceFile 以 `@param{N}` 参数化，restore 时按参数表回填实际
  路径；magic "MXTPL" + formatVersion=1，拒绝未知版本。
- `mCurveTemplateManager`（单例）：模板文件读写、`templateRecents`（QSettings，
  时间正序/插尾去重/上限 20）、`readTemplateMeta`（模板名+参数清单提取）。

### 应用管线（mTemplateApplyPipeline，无 GUI 依赖可无头单测）

- 三模式：**追加**（模板页整体追加在后）/ **替换**（目标组既有曲线/基准线/
  注释删除，图表骨架保留回放新环境与曲线）/ **叠加**（曲线重放到目标组既有
  图表，同名自动改名 "名 (N)" 保证唯一，不回放页面/图表环境）。
- 一个撤销宏 `beginMacro("模板曲线-<模板名>")` 包全部结构命令，Ctrl+Z 一次
  整体撤销（数据直写不随 redo 重放，与 pch 导入同源限制）。
- 来源分派：文件来源按五元组 `assembleChannelData` 装配（失配跳过不产假数据）；
  数值来源定义即数据直写；数学来源重映射引用后重新求值。
- 通道策略（收口在 assembleFileCurve 缝隙）：0 跳过验证=按模板五元组直装；
  1 校验通道=按映射表装配；2 选择最佳匹配=精确失配时按归一化相似度自动配对
  （阈值 0.6，无命中告警跳过）。
- 应用成功后向 `mCurveLayerManager` 登记层（层名=模板名）。

### 通道映射（mChannelMapCore + mChannelMapDlg）

- `mChannelMapCore`（无 GUI 依赖）：模板通道行提取（仅文件来源）、数据文件
  预登记（行集引用路径同步入共享仓库，首次打开即可匹配/手选——此前须先应用
  一次模板入库，否则精确匹配/相似预配/候选枚举全哑、整表红叉）、精确匹配、
  相似预配（自动填充档：文件名=全字段/后缀名=只比结果类型+分量）、最佳匹配
  （只动未匹配行）、四级候选枚举、CSV 保存/导入（UTF-8 BOM，按曲线名对齐）。
- `mChannelMapDlg`：五列表（文件名称/工况/结果类型/对象/分量），状态标识缀于
  四列单元格值后（未匹配红✗→匹配灰✓→确认绿✓）；四列单击弹出候选菜单（单元
  格不放编辑控件，候选打开时按该文件层级现算，改配即时试装配）；显示通道三态过滤；自动连接相似
  通道默认勾选（预配档=文件名）；确认在工具栏、不关闭。入口两处：模板曲线
  对话框"勾"按钮（预开，
  结果按模板缓存）与"校验通道"策略应用时自动弹。

### 图层（mCurveLayerManager + mLayerManageDlg）

- `mCurveLayerManager`（单例，会话内存活，不随 mxses 持久化）：层=一次应用的
  曲线/基准线/页面集合；读接口现查文档剪除失效成员；显隐复用既有 `kVisible`
  键（与曲线树同键，图表/视图联动免费获得），勾选态现算不存储；删除层=成员
  removeNode 包单个撤销宏（撤销恢复节点但层记录不回收，v1 接受）。
- `mLayerManageDlg`（非模态单例）：两级树（需求图）：组="Page(s): 页号并集"
  （可勾=整组显隐）→ 层="Layer N: 层名"（勾=任一成员曲线可见，现算）；
  层节点右键重命名/删除；刷新整树重建；分支绘制用曲线树同款虚线导引
  （DashBranchTreeWidget → PaintDashBranches）。（实现偏差：QTreeWidget 而非
  设计的 QAbstractItemModel，小数据量快速达成。）

### 测试

tests/PostCurve 九个测试目标（本地，不入库）：Serialize / TemplateManager /
LayerManager / ApplyPipeline / ChannelMapCore / StatsBandCount / BarExtract /
ConvertBar / ExportStats 脚本，全部无头（QCoreApplication+pch fixture 注入
TEST_PCH_PATH，缺失 QSKIP）。对话框交互为 6.1 手工回归清单。
