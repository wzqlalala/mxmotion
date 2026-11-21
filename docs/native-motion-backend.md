# OCC 原生运动模型求解接入

## 输入与职责

原始 `GUI/src/MainWindow/mMotionSolverThread.cpp` 是完整的 Adams 容器输入桥接，不能通过把原始工程压缩为旋转副、移动副、固定副三种类型来替代。

本实现保留原 `.motion.json` 的 `{formatVersion, exportTime, motionData}` 文档，包括未知扩展字段。`NativeMotionDocument::bodyPartBindings` 单独记录原运动体 ID 与一个或多个 OCC 部件 ID 的对应关系。原 ID `0` 有效，大地为 `-1`，不使用轻量模型中的大地 ID `0` 约定。

`prepareNativeMotion` 在文档线程准备质量属性、离散曲面、精确曲线采样及 BREP 快照。`solveNativeMotion` 使用不可变快照创建 MotionEngine 的真实 Adams 容器和系统，工作线程不读取当前 CAD 文档。原轻量模型仍可独立求解。

相关实现：

- `ModelData/src/MotionOcc/NativeMotionDocument.*`：原文档、引用验证及几何绑定。
- `NativeMotionGeometry.*`：OCC 质量属性、曲面/边采样、柔性体输入快照。
- `NativeMotionSolver.cpp`：Adams 输入、求解器配置、初始装配、取消及补充结果记录。
- `NativeMotionProjection.cpp`：按真实残差停止的位置投影及接受帧的物理约束检查。
- `OccTessellation.*`：保持精确面/拓扑不变的完整三角化；失败的相切平面轮廓通过同几何分割恢复网格。
- `NativeMotionPid.cpp` / `NativeMotionSensors.cpp`：闭环控制与同源传感器计算。
- `NativeMotionFlex.cpp`：Netgen 自动体网格、真实模态缩减与变形节点采样。
- `NativeMotionResults.cpp`：统一时间轴和原单位的完整结果通道。

## 能力对应

| 原始对象 | OCC 求解路径 |
| --- | --- |
| 刚体、固定体、初始线速度/角速度、质量/质心/惯量 | `adams_part_struct`；多部件绑定与初始全局几何位置保持一致 |
| Revolute / Translational / Cylindrical / Spherical / Fixed | 原生对应关节 |
| Universal / Hooke | `adams_universal_joint_struct`，独立轴线与十字销方向 |
| PointPlane / PointLine / Planar / Parallel / Perpendicular / Direction | 原生几何/方向约束 |
| ConstantVelocity / Screw | 等速及螺纹导程原生约束 |
| PointOnSurface / PointOnCurve / CurveOnCurve | OCC 曲面网格、弧长均匀采样、切向/曲率及开闭环标志；滚动/滑动设置传给引擎 |
| 齿轮、齿轮齿条、双向线缆耦合 | `adams_gear_joint_struct` / `adams_rackpin_joint_struct` / `adams_cable_joint_struct` |
| 位置/速度驱动 | 常数、有效表达式或时间表；旋转输入从度转换为弧度，保持表格优先规则 |
| 线性、扭转、体上两点弹簧 | 线性、表格、力函数、阻尼函数、预载及安装/自由长度；按真实端点创建线性副弹簧 |
| 矢量力/力矩 | 常数/表达式，实际作用点及全局/随体方向 |
| 标量力/力矩、三点力/力矩 | 原生两点及独立方向参考点力元 |
| 衬套 | 六分量刚度、阻尼、预载、安装偏置 |
| 3D 接触 | 绑定实体的真实 OCC 三角网格；IMPACT 参数、摩擦、凸包选项 |
| 用户 Marker | 初始全局位置与完整四元数姿态 |
| 传感器 | 体、关节反力、驱动输出、两点距离/速度/加速度、连续角度；全局或参考体坐标系 |
| PID 与控制输入 | 稳定 ID 定位速度电机；常数、多项式、自定义曲线；前馈、测量微分、抗积分饱和与输出限幅 |
| AutoFlex | 材料 + 四面体网格 → `modal::BuildFlexBody`；无网格时调用 OCC/Netgen 工作进程；逐帧恢复真实模态节点 |
| 新只拉绳索 | 独立 `ADMTensionCable`；松弛不受力，拉力非负，阻尼卸载不产生推力 |
| 新独立止挡 | 独立 `ADMSoftStop`；上下侧、全局/随体轴向、幂律或表格、阻尼渐入与非粘附回弹 |

`Line`、`Centripetal` 在原线程中已经显式中止；通用 `Coupling` 只有枚举/字符串映射，没有实际引擎约束分支。裸 `jointType=Gear` 也只有一个共享原点，缺少两齿轮轴心定义，原线程还未创建引擎要求的派生参数结构。它们会返回清楚的失败信息，不会删掉约束后继续求解，也不会凭空假设重合的齿轮轴心。有效齿轮传动使用 `couplings` 集合中的 `Gear`，引用两侧真实旋转副。`Acceleration` 驱动也没有现有 Adams 容器接口，原线程同样不支持。

原力元枚举中的 `TireForce`、`ForceField` 等名称没有原线程的独立实现；原代码把所有非 `Force`/`Bushing` 矢量对象当作力矩。新接入拒绝这种含义错误的替代。普通弹簧的 `PositiveOnly`/`NegativeOnly` 标志没有原 Adams 容器语义，原线程未传递这些标志；需要单边连接时使用有独立力学定义的只拉绳索或止挡。

## 单位与已修复的数值问题

原文档保存毫米、千克、秒；惯量为 `kg·mm²`，重力为 `mm/s²`。用户力是 `N`，力矩是 `N·mm`，旋转角是度。原生引擎的相应一致单位是 `mN`、`mN·mm`、弧度。

| 数值 | 输入到引擎 | 输出给界面 |
| --- | --- | --- |
| 长度、线速度、线加速度 | 原值 | mm、mm/s、mm/s² |
| 力、线性刚度/阻尼 | ×1000 | 力 ×0.001 → N |
| 力矩 | ×1000 | ×0.001 → N·mm |
| 旋转角/角速度 | ×π/180 | ×180/π |
| 扭簧/旋转限位刚度、阻尼（每度） | ×1000×180/π | 相应的度量通道 |
| 能量、功率 | — | ×10⁻⁶ → J、W |
| 柔性材料 E（N/mm²）、ρ（kg/mm³） | E ×1000，ρ 原值 | 同一 mm–kg–s 体系 |

### 惯量交叉项的输入约定

惯量数组始终按 `[Ixx, Iyy, Izz, Ixy, Ixz, Iyz]` 排列，但历史文件对最后三个值存在两种定义。MotionEngine 的 `SetInertiaXY` 直接填入对称惯量矩阵的交叉项，采用负惯量积；部分原文件导出的是正惯量积 `+∫xy dm`、`+∫xz dm`、`+∫yz dm`。两者不能只凭数组名区分。运动体可用 `inertiaConvention` 明确指定，默认值为 `auto`：

| 约定 | 准备求解时的处理 |
| --- | --- |
| `tensor` | 六个值直接组成对称惯量矩阵，不改变交叉项符号 |
| `products` | 保留前三个对角值，将后三个正惯量积取反后组成矩阵 |
| `auto` 或缺省 | 按下面的声明与 CAD 证据优先级处理；无证据时沿用 `tensor` |

优先级如下：

1. 运动体显式的 `tensor` / `products` 优先于文档级声明和 CAD 推断。
2. `auto` 时读取原根对象的 `generatorNotes.coordinateAndInertiaConvention.inertia`。仅当其中包含精确六分量公式 `[Ixx,Iyy,Izz,-CAD_tensor_xy,-CAD_tensor_xz,-CAD_tensor_yz]` 时，按原文档已经声明的 `products` 解释。匹配只去除空白，不根据自然语言猜测；因此自定义质量、排除显示零件的质量模型，以及没有 CAD 的完整质量属性也可遵循这条明确声明。
3. 没有上述声明时，将该体全部绑定 CAD 实体聚合，以源质量缩放其惯量，并核对源质心和全部六个分量。只有 `products` 唯一匹配、`tensor` 不匹配时才恢复历史正惯量积约定。局部部件吻合、质心不符、仅主惯量吻合或近零交叉项带来的歧义都不足以自动翻转。
4. 没有明确约定或充分证据时保留原矩阵解释，仍执行正定性与主惯量三角不等式检查。不因检查失败而试探性翻转符号。

这一处理只改变不可变求解快照中的矩阵，不改写原 `.motion.json` 的惯量数字、单位或 `generatorNotes` 元数据。结果的 `diagnostics` 记录转换来自运动体显式设置、原文档公式或完整 CAD 指纹。缺失惯量而从 CAD 补算的矩阵本身已采用 `tensor`，不再次取反；界面从 CAD 新建的运动体也明确保存 `tensor`。

回归使用两个独立 CAD 实体合成的斜置长方体，三个交叉项均非零，独立检查解析惯量矩阵及施加力矩后的角加速度。还检查显式覆盖、质心不匹配时保留原值、无证据的无效张量拒绝，以及无 CAD 的自定义体按精确文档声明求解，并断言原数字和元数据完整保留。

### 装配、约束与时间轴

初始装配先解位置与纯运动学速度投影，再估计加速度，并恢复真正的零时刻速度。引擎原装配函数使用 `10⁻⁶ s` 虚拟步估算加速度；直接沿用其速度会给自由体凭空增加初速度。解析恒力回归检查这一点。

有运动副的原生模型默认在每个接受的时间步执行位置投影，以实际约束残差提前停止，最多 50 次。每步只准备一次方程描述，使用原质量加权约束方程，不增加虚拟动力学时间。固定三次迭代不足以处理小惯量、多重闭环的机构；提高收敛预算后才能保持机构闭合。源方案可明确设置 `positionAssemblyPerStep: false` 对照旧行为，或用 `positionAssemblyIterations`（1–50）控制上限。用户选择的积分器和方程求解器不因此被替换。柔性体继续使用原桥接的 MINRES/线性隐式 Euler 路径。

每个接受帧还独立检查可比较的真实关节闭合距离（mm）、轴/销方向角（度）及驱动约束误差。超过验收阈值时返回具体方案、关节或驱动、物理时刻、误差及单位，不把互相矛盾的模型作为成功动画。失败不会覆盖调用方已有的结果对象；界面按自己的结果失效规则管理旧结果。

输出时刻由整数步号计算，并在 PID、位置投影和记录之前同时校正系统与积分器时钟。这样 `15 s / 0.001 s` 正好有 15001 帧（含初始帧），不会因浮点累积误差多积分一个约 `10⁻¹² s` 的尾步。终时不是步长整数倍时，仍保留真实缩短的最后一步。

原旋转软限位把“每度”刚度乘以 `π/180`，方向颠倒；本实现与扭簧一致按 `180/π` 换算。硬单边限位与线性求解器不兼容时明确中止，即使限位在初始状态尚未接触。

加载原文件时保留求解器枚举，但仅运行当前构建真正实现的类型。最新引擎的系统工厂尚未实现 `ADDM`、`BICGSTAB`；`PARDISO`/`MUMPS` 则需要相应第三方构建。选择这些不可用算法会明确失败，而不会沿用默认 PSOR。初始化后还核验实际安装的求解器/积分器与生效配置一致。

## 几何与柔性体补充数据

原 `.motion.json` 不包含 CAD 实体，也不包含原几何引擎的拓扑 ID 到新 OCC 拓扑的可靠映射。部件可通过名称/实例及几何质量属性确认绑定。曲线/曲面选择可用如下明确绑定，索引是该 OCC 部件中对应类型的一基索引：

```json
{
  "occSurfaceFaces": [{"partId": 7, "index": 3}],
  "occCurveEdges": [{"partId": 7, "index": 2}],
  "occCurveIEdges": [{"partId": 8, "index": 1}],
  "occCurveJEdges": [{"partId": 7, "index": 4}]
}
```

仅点在面上副在原始 `surfaceFaceIds` 为空时可以使用整个宿主体表面。非空且无法恢复的原拓扑选择必须重新绑定，不能把旧 ID 猜成 OCC 索引。

保留原线程的初始贴合检查，使用 OCC 的实际裁剪面/边计算最近距离，避免将采样相位或弦高误差当成真实几何分离。PointOnSurface 容差为 `max(0.5 mm, 0.5 × 最大三角形边长)`，PointOnCurve 为 `max(0.1 mm, 0.002 × 曲线长度)`，CurveOnCurve 为 `0.2 mm`。超过容差时明确拒绝，并给出距离与阈值；不会让初始装配把错误放置的对象瞬间拖动到约束位置。

CurveOnCurve 使用新引擎真实的无质量曲线坐标与滚动/滑动方程，需要 `SPARSE_QR`、`SPARSE_LU` 等完整直接求解器；不把 PSOR 之类质量逆迭代器偷偷替换成另一种求解器。选择不兼容的组合会收到引擎的明确诊断。

若点在面上/点在线上副的作用点滑出真实几何边界，引擎会正常保留此前的接受帧。结果标为 `ResultTermination::GeometryBoundary`，诊断说明越界运动副、实际终止时刻和距离；动画与曲线使用实际结果时间范围。没有边界原因却提前结束的求解明确失败，不能标为正常到达终时。

原 AutoFlex 导出只有 `materialId`，没有材料数据库。可给 AutoFlex 对象补充明确的材料记录：

```json
{
  "material": {"youngModulus": 210000, "poissonRatio": 0.3, "density": 0.00000785},
  "elementSize": 4,
  "modeCount": 6,
  "connectionRadius": 3
}
```

没有体网格时从已绑定 CAD 自动生成。也可给出相对原工程目录的 Netgen `meshFile`，或嵌入 `mesh: {nodes: [[x,y,z], ...], tetrahedra: [[i,j,k,l], ...]}`；嵌入网格连接从零开始。`attachGroups` 是显式零基网格节点组；否则按运动副位置和连接半径建立附着组。原 `attachPoints` 是原网格全局节点 ID，需要随网格提供逐节点的 `mesh.nodeIds`，或重新指定 `attachGroups`/初始全局坐标的 `attachPointPositions`。缺少这一对应关系时明确中止，不忽略原附着约束。

求解期间捕获模态坐标和 `SetInternalStateWithModes` 恢复的真实节点位置，动画使用这些节点与引擎表面三角形，不用假造的解析弯曲形状替代求解结果。

采用最新 `FlexBodyBuilder` 的外接口语义：目标体保持原 COM 参考位置，其现有 Marker 与运动副不迁移。最新引擎已经明确回退旧 GUI 的“把参考系平移到附着点”做法，因为那会破坏已装配的约束。Builder 将接口体从顶层移入模态装配，去重后清零接口体自身质量/惯量，由 FEM 提供真实柔体质量，避免同一刚体变量或质量被重复计入。现有模态回归的 COM 为 `(5,5,5)`，端部固定点为 `(0,5,5)`，附着组位于 `x=0`，已经包含偏离 COM 的附着。

## 新止挡和只拉绳索的文档格式

对象位于 `motionData.tensionCables` 和 `motionData.softStops`。`bodyIId`、`bodyJId` 是不同的原运动体 ID，其中一端可以为大地 `-1`；`localI`、`localJ` 延续原力元输入约定，是初始全局坐标。`enabled` 缺省为 true。

```json
{
  "tensionCables": [{
    "id": 0, "name": "只拉绳索", "enabled": true,
    "bodyIId": 0, "bodyJId": -1,
    "localI": {"x": 0, "y": 0, "z": 0},
    "localJ": {"x": 100, "y": 0, "z": 0},
    "freeLength": 95, "stiffness": 20,
    "damping": 0.1, "engagementWidth": 0.2
  }],
  "softStops": [{
    "id": 0, "name": "上侧止挡", "enabled": true,
    "bodyIId": 0, "bodyJId": -1,
    "localI": {"x": 0, "y": 0, "z": 0},
    "localJ": {"x": 0, "y": 0, "z": 0},
    "direction": {"x": 1, "y": 0, "z": 0},
    "directionMode": 1, "side": 1, "stopPosition": 10,
    "stiffness": 20, "exponent": 1.5,
    "damping": 0.1, "dampingDepth": 0.2,
    "hasTable": false, "tableX": [], "tableY": []
  }]
}
```

方向模式 1 为全局固定，2 跟随 I，3 跟随 J；方向输入始终为初始全局方向。`side=1` 为上侧，`-1` 为下侧。表格横轴是非负压入量 mm，纵轴是非负恢复力 N，必须从 `(0,0)` 开始且横轴严格递增。

只拉绳索是无质量可伸长力元，不是不可伸长约束，也不是原双向线缆位移耦合。止挡是独立力元，不额外锁定其余自由度。

## 验证入口

`MotionOccNativeSmoke` 是解析量和真实引擎回归，覆盖初始时刻、原 ID 0、单位换算、驱动、传感器、约束族、力元、PID、OCC 几何约束/接触、单边力、软限位、取消及真实模态/自动网格。还包括长时间精确时间轴、冗余六体闭环、矛盾约束拒绝、三类几何初始贴合拒绝、几何边界终止，以及独立生成的内切圆环面三角化（原位、旋转平移和反向面）。CurveOnCurve 回归实际使用 CAD 圆柱边与盒体直边，并施加非零切向力，对照纯滚动和允许滑动时的材料点滑移、角速度和转动惯性加速度，防止“约束缺失但静止结果看起来正确”的假通过。

`MotionOccNativeCases` 用原始 CAD + `.motion.json` 执行完整原方案，并独立检查帧时间、有限数值、固定体、关节闭合和恒速驱动误差。真实案例失败会写入报告，不会通过缩短时长或删除约束伪装通过。源模型确有矛盾时，可以使用单独诊断副本验证原因，但不覆盖用户原文件，也不把修改后的结果当作原例通过。
