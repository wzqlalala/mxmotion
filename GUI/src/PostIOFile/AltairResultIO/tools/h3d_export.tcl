# =====================================================================
# h3d_export.tcl — HyperView 批处理导出：H3D -> .hwascii + .fem
#
# 手工运行（102 测试机）:
#   cd /d <本文件目录>
#   "E:\SoftwareInstall\HyperWorks2022\hwdesktop\hv\bin\win64\hyperview.exe" -b -tcl h3d_export.tcl
#   （先改下面 CONFIG 段的四个路径；全程纯 ASCII 路径，许可证同 hvtrans）
#
# ⚠️ 运行时产品版本内嵌于 hyperview_job.cpp（生成 export.tcl 时只替换 CONFIG 段），
#    修改本脚本逻辑后必须同步该模板，两者保持一致。
#
# 设计说明：
#   1. HyperView 没有官方"h3d→hwascii/fem"一键命令；本脚本用官方 Tcl API
#      （2022 帮助文档 + 102 真机探针已核实的 poI* 调用链）遍历模型与结果
#      手工写出两个文本文件：
#        会话链:  hwi → session → project → page → window → client(poIPost)
#        打开:    client AddModel（2022 唯一打开路径，OpenFile 仅旧版兜底）
#        网格:    model(poIModel) → AddSelectionSet + "Add all" 全选（v4 探针
#                 定案：批处理下 Query 必须绑非空选择集，否则迭代器 0 行）
#                 → QueryCtrl SetSelectionSet → SetQuery "node.id node.coords"
#                 / "element.id element.config element.connectivity"
#                 → GetIteratorHandle → Valid/GetDataList/Next
#        结果:    model SetResult 挂结果库（v5 探针验证中）→ ResultCtrl(子工况/
#                 步/类型枚举) + ContourCtrl(设云图)
#                 → QueryCtrl "<entity>.id contour.value" 逐列取值
#   2. hwi 语义：GetSessionHandle sess 之后的 "sess" 是句柄【命令名】（官方示例
#      全部裸名调用）。本脚本统一用 acquire() 把句柄名镜像进同名变量后以 $name
#      调用——出参别名式与返回值式的 hwi 实现都兼容。
#   3. 个别方法签名（OpenFile/AddModel、SetComponent 等）跨版本有变体，脚本对
#      每个这类操作按"官方形式优先、变体兜底"逐个尝试（try_forms），命中的形式
#      与失败详情全部进日志：一次运行要么成功、要么留下精确的二轮定位证据。
#   4. 成功判定：.fem 有 GRID、.hwascii 有结果组时，日志末尾写 EXPORT_OK；
#      调用方（hyperview_job.cpp / 手工检查）以该标记为准，不信任退出码。
#   5. v1 已知限制（均记日志）：复数结果(M/P、R/I)与 node_on_element 绑定跳过；
#      beam 单元方向矢量写默认值；内存上限 MAXCELLS 之外的大结果组跳过。
# =====================================================================

# ----------------------------- CONFIG -------------------------------
set H3D_IN    "in.h3d"
set OUT_HWA   "out.hwascii"
set OUT_FEM   "out.fem"
set LOG_PATH  "hyperview_log.txt"
# --------------------------- CONFIG END -----------------------------

# GUI Tcl 控制台（Tk main console）没有 stdout 通道；批处理有。
set HAS_STDOUT [expr {[lsearch -exact [file channels] stdout] >= 0}]

proc export_log {msg} {
    global HAS_STDOUT
    if {$HAS_STDOUT} {
        puts "H3DEXPORT: $msg"
        catch { flush stdout }
    } else {
        catch { puts $msg }
    }
    global LOG_CH
    if {[info exists LOG_CH] && $LOG_CH ne ""} {
        puts $LOG_CH $msg
        flush $LOG_CH
    }
}

# Altair 句柄式 API 的子命令枚举（调用不存在的方法时，报错文本枚举全部合法子命令）
proc dump_methods {handle name} {
    if {[catch {$handle __no_such_method__} msg]} {
        export_log "==== subcommands of $name ===="
        export_log $msg
    }
}

# 依序尝试同一操作的多种调用形式（官方形式优先，变体兜底），命中即记录形式；
# forms 为命令串列表，在调用方作用域 evaluate
proc try_forms {label forms} {
    foreach f $forms {
        if {![catch {uplevel 1 $f} res]} {
            export_log "$label OK: [string map [list "\n" " ; "] [string trim $f]]"
            return [list 1 $res]
        }
    }
    export_log "$label FAILED (tried [llength $forms] forms)"
    return [list 0 ""]
}

# 出参句柄获取统一化：命中后把句柄名（出参名或返回值）镜像进 $outName 变量，
# 并验证该句柄命令真实存在；返回 1=成功。
# 注意：try_forms 用 uplevel 1（调用方作用域），acquire 不能委托它（会错位到
# acquire 自己的作用域），这里必须内联同样的尝试循环。
proc acquire {label forms outName} {
    upvar 1 $outName out
    foreach f $forms {
        if {![catch {uplevel 1 $f} res]} {
            export_log "$label OK: [string map [list "\n" " ; "] [string trim $f]]"
            set out [string trim $res]
            if {$out eq ""} { set out $outName }
            if {[info commands $out] eq ""} { break }
            return 1
        }
    }
    export_log "$label FAILED (tried [llength $forms] forms)"
    return 0
}

proc fmt10 {v} { return [format %.10g $v] }

# GUI 同款应用（postDisCommand::prcApplyContour）：重设当前步 + 显示选项 + Draw。
# 云图定义（SetDataType/SetDataComponent）后必须走这一步值才会落地。
# Apply 后重申当前仿真步，防止 animator 步覆盖循环目标步。
proc hv_apply_contour {anName rcName si} {
    if {$anName ne "" && [info commands $anName] ne ""} {
        catch { $anName SetCurrentStep [$anName GetCurrentStep] }
    }
    if {[info commands client] ne ""} {
        catch { client SetDisplayOptions "contour" true }
        catch { client SetDisplayOptions "legend" true }
    }
    if {$rcName ne "" && [info commands $rcName] ne ""} {
        catch { $rcName SetCurrentSimulation $si }
    }
    if {[info commands client] ne ""} {
        catch { client Draw }
    }
}

# ---- Measure 值读取（GUI measureinterpolation.tcl 官方路径）----
# hex8(208) 三角剖分（GUI GetTriaFaces 同款，索引指向连接表）
proc hex8_trias {conn} {
    set idx {{0 1 5} {0 5 4} {1 2 6} {1 6 5} {2 3 7} {2 7 6} {3 0 4} {3 4 7} {4 5 6} {4 6 7} {3 2 1} {3 1 0}}
    set t {}
    foreach tri $idx {
        set tria {}
        foreach i $tri { lappend tria [lindex $conn $i] }
        lappend t $tria
    }
    return $t
}

# 按连接表建立 节点→测量三角 映射（节点为三角首顶点，插值参数 0 0 0 = 值即节点值）。
# hex8 做 8 次循环旋转，让全部 8 个局部位置都能轮转到首顶点
#（否则排后位的节点在某些网格上永远不被覆盖）。
proc build_node_measure_map {} {
    global g_elemConn g_nodeTria
    set g_nodeTria [dict create]
    dict for {eid conn} $g_elemConn {
        if {[llength $conn] != 8} { continue }
        for {set i 0} {$i < 8} {incr i} {
            set rot {}
            for {set j 0} {$j < 8} {incr j} {
                lappend rot [lindex $conn [expr {($j + $i) % 8}]]
            }
            set n0 [lindex $rot 0]
            if {![dict exists $g_nodeTria $n0]} {
                dict set g_nodeTria $n0 [list [lindex $rot 0] [lindex $rot 1] 0 [lindex $rot 0] [lindex $rot 5] 0 0]
            }
        }
    }
}

# Measure 批量取值：建测量 → 每节点 AddNode → GetValueList → 按 AddNode 顺序对齐
# 返回 {n vals}；失败返回 {-1 {}}
proc collect_via_measure {clientH mi prefix key poolComp compId poolNode nodeIds} {
    global g_nodeTria
    set measName "mea_[incr ::iterSeq]"
    set measId ""
    if {[catch {
        set measId [$clientH AddMeasure]
        $clientH GetMeasureHandle $measName $measId
        $measName SetLabel "h3dexport_[incr ::iterSeq]"
        $measName SetType "Nodal Contour"
    } msg]} {
        export_log "  measure create FAIL: $msg"
        return [list -1 {}]
    }
    set addList {}
    set order {}
    foreach node $nodeIds {
        if {[dict exists $g_nodeTria $node]} {
            lappend addList [list $mi $poolComp $compId $poolNode {*}[dict get $g_nodeTria $node]]
            lappend order $node
        }
    }
    set added 0
    set firstErr ""
    foreach args $addList {
        if {![catch { $measName AddNode $args } m]} {
            incr added
        } elseif {$firstErr eq ""} {
            set firstErr [string range $m 0 120]
        }
    }
    catch { $clientH Draw }
    set vals {}
    catch { set vals [$measName GetValueList] }
    export_log "  measure '$prefix': added=$added vals=[llength $vals] first=[lrange $vals 0 2] firstErr=$firstErr"
    # 用完即删（按 label 前缀清理，避免会话内累积）
    catch {
        foreach mid [$clientH GetMeasureList] {
            set tmpN "meatmp_[incr ::iterSeq]"
            if {![catch { $clientH GetMeasureHandle $tmpN $mid } m2]} {
                if {[string first "h3dexport_" [$tmpN GetLabel]] == 0} {
                    $clientH RemoveMeasure $mid
                }
                catch { $tmpN ReleaseHandle }
            }
        }
    }
    if {[llength $vals] != [llength $order]} {
        export_log "  measure vals/order mismatch: vals=[llength $vals] order=[llength $order]"
    }
    set nv [expr {[llength $vals] < [llength $order] ? [llength $vals] : [llength $order]}]
    set out {}
    for {set i 0} {$i < $nv} {incr i} {
        lappend out [list [lindex $order $i] [lindex $vals $i]]
    }
    catch { $measName ReleaseHandle }
    return [list $nv $out]
}

# v4 真机定案：批处理下 Query 必须绑定非空选择集（官方 poiqueryctrl_setselectionset
# 示例），全选形式经扫描证实为 `Add all`。建集失败不致命：仅记录并按裸查询兜底。
array set ssIds {node {} element {}}
array set ssHn {node {} element {}}
proc bind_ss {q t} {
    global ssIds
    if {![info exists ssIds($t)] || $ssIds($t) eq ""} { return 0 }
    if {[catch {$q SetSelectionSet $ssIds($t)} msg]} {
        export_log "WARN: SetSelectionSet($t): $msg"
        return 0
    }
    return 1
}

proc init_selection_sets {model} {
    global ssIds ssHn
    foreach t {node element} {
        set hn "ss_$t"
        if {[catch {$model AddSelectionSet $t} id]} {
            export_log "WARN: AddSelectionSet $t: $id"
            continue
        }
        if {[catch {$model GetSelectionSetHandle $hn $id} msg2]} {
            export_log "WARN: GetSelectionSetHandle($t): $msg2"
            continue
        }
        if {[catch {$hn Add all} msg3]} {
            export_log "WARN: $hn Add all: $msg3"
            continue
        }
        set ssIds($t) $id
        set ssHn($t) $hn
    }
    export_log "selection sets: node=$ssIds(node) element=$ssIds(element)"
}

# HyperView element.config → Nastran 卡名与期望连接数；不支持的构型返回空
proc elem_card {cfg} {
    set c [string trim $cfg]
    # v7.x 真机：H3D element.config 返回数字码（208=hex8 已实测，8 节点连接）
    switch -- $c {
        208 { return {CHEXA 8} }
        207 { return {CHEXA 20} }
        206 { return {CPENTA 6} }
        205 { return {CPYRAM 5} }
        204 { return {CQUAD4 4} }
        203 { return {CTRIA3 3} }
        202 { return {CROD 2} }
    }
    switch -glob -- [string tolower $c] {
        *tetra10* { return {CTETRA 10} }
        *tetra4*  { return {CTETRA 4} }
        *hex20*   { return {CHEXA 20} }
        *hex8*    { return {CHEXA 8} }
        *penta6*  { return {CPENTA 6} }
        *pyra5*   { return {CPYRAM 5} }
        *tria3*   { return {CTRIA3 3} }
        *quad4*   { return {CQUAD4 4} }
        *rod2*    { return {CROD 2} }
        *beam3*   { return {CBEAM 3} }
        *rod*     { return {CROD 2} }
        *beam*    { return {CBEAM 3} }
    }
    return {}
}

# 执行 "<prefix>.contour.value" 查询并收进字典 V(key)(id)=value；返回行数（-1 出错）。
# v7.7 真机：查询字段 node.contour.value 命中，但行形态未证实——两种都容：
#   {id value...} 直接入字典；纯值行（无 id 列）按选择集 GetList 顺序对齐 id。
# 迭代一个查询迭代器并把行折进 V：≥2 列取 [第一列,第二列]；单列行累积进 valonly
# （v7.7：行形态未证实，两种都容）。返回 {n valonly rows}，首两行写日志。
proc contour_ingest {iter vname key tag} {
    upvar 1 $vname V
    set n 0
    set valonly {}
    set rows 0
    while {[$iter Valid]} {
        set row [$iter GetDataList]
        if {$rows < 2} { export_log "  contour rows($tag) #$rows: <$row>" }
        incr rows
        if {[llength $row] >= 2} {
            dict set V $key [lindex $row 0] [lindex $row 1]
            incr n
        } else {
            foreach v $row { lappend valonly $v }
        }
        $iter Next
    }
    return [list $n $valonly $rows]
}

# 单列纯值行：按选择集 GetList 顺序对齐 id 后入 V；返回写入数
proc zip_valonly {vname key prefix valonly} {
    upvar 1 $vname V
    global ssHn
    set ids {}
    if {[info exists ssHn($prefix)] && [info commands $ssHn($prefix)] ne ""} {
        catch { set ids [$ssHn($prefix) GetList] }
    }
    export_log "  zip valonly: vals=[llength $valonly] ids=[llength $ids]"
    set nv [expr {[llength $valonly] < [llength $ids] ? [llength $valonly] : [llength $ids]}]
    for {set i 0} {$i < $nv} {incr i} {
        dict set V $key [lindex $ids $i] [lindex $valonly $i]
    }
    return $nv
}

proc collect_values {q prefix key vname} {
    upvar 1 $vname V
    global ssHn
    bind_ss $q $prefix
    set n 0
    set raw 0
    set valonly {}
    foreach qf [list "$prefix.contour.value" "$prefix.id $prefix.contour.value"] {
        if {[catch {$q SetQuery $qf} msg]} { export_log "  SetQuery '$qf' failed: $msg"; continue }
        set itName "iter[incr ::iterSeq]"
        if {![acquire "  iterator" [list "\$q GetIteratorHandle $itName" {$q GetIteratorHandle}] $itName]} { continue }
        lassign [contour_ingest $itName V $key "$prefix '$qf'"] n valonly raw
        catch { $itName ReleaseHandle }
        export_log "  contour '$qf': n=$n raw=$raw valonly=[llength $valonly]"
        if {$n > 0 || [llength $valonly] > 0} { break }
    }
    if {$raw > 0 && $n == 0 && [llength $valonly] == 0} {
        # 全空行诊断：对照字段（故意乱写）区分"字段名不校验"vs"真实字段缺物化"
        set ctl -2
        if {![catch {$q SetQuery "bogus.control.field"} m1]} {
            set itName "iter[incr ::iterSeq]"
            if {![catch {$q GetIteratorHandle $itName} m2]} {
                set ctl 0
                while {[$itName Valid] && $ctl < 1000000} { incr ctl; $itName Next }
                catch { $itName ReleaseHandle }
            }
        }
        export_log "  DIAG: bogus control field rows=$ctl (contour raw=$raw; ctl==raw → 字段名不校验,查询读值无效; ctl==0 → 字段有效但值未物化)"
        if {$ctl <= 0} {
            # 字段名校验通过 → 物化电池：DrawFrame/Draw + 等待后复查
            export_log "  DIAG: trying materialization (DrawFrame/Draw + wait)"
            if {[info commands client] ne ""} {
                catch { client DrawFrame }
                catch { client Draw }
            }
            after 800
            if {![catch {$q SetQuery "$prefix.contour.value"} msg]} {
                set itName "iter[incr ::iterSeq]"
                if {![catch {$q GetIteratorHandle $itName} m2]} {
                    lassign [contour_ingest $itName V $key "after-draw $prefix"] n valonly rows
                    catch { $itName ReleaseHandle }
                    export_log "  after-draw: n=$n rows=$rows valonly=[llength $valonly]"
                    if {[llength $valonly] > 0} {
                        incr n [zip_valonly V $key $prefix $valonly]
                    }
                }
            }
        }
    }
    if {[llength $valonly] > 0 && $n == 0} {
        incr n [zip_valonly V $key $prefix $valonly]
    }
    return $n
}

if {[catch {set LOG_CH [open $LOG_PATH w]}]} { set LOG_CH "" }
set FAILED 0
set iterSeq 0

# 版本戳:判定部署侧 DLL/脚本是否为新版(缺这些行 = 旧版在跑)
set SCRIPT_BUILD "2026-10-12-B (measure primary + rotation map + sci10 + preserve-on-fail)"
if {![info exists JOB_EXE]} { set JOB_EXE "(manual)" }
if {![info exists BUILD_STAMP]} { set BUILD_STAMP "(manual)" }

export_log "=== h3d export start: $H3D_IN ==="
export_log "script build: $SCRIPT_BUILD"
export_log "job exe: $JOB_EXE"
export_log "cpp build stamp: $BUILD_STAMP"
export_log "now: [clock format [clock seconds] -format %Y-%m-%d_%H:%M:%S]"

# ---- 1) 会话句柄链：session -> project -> page -> window -> HyperView client
#         （官方语义：链上每个 Get*Handle 的出参是句柄命令名）
# SetClientType 拼写跨版本可能有差异："HyperView" 失败时用 "HV" 重试
# （此时链上其余句柄已建好，仅重试类型设置与 client 获取）
set client ""
set r [try_forms "session handle chain" [list {
    hwi OpenStack
    hwi GetSessionHandle sess
    sess GetProjectHandle proj
    set pageId [proj GetActivePage]
    proj GetPageHandle page $pageId
    set winId [page GetActiveWindow]
    page GetWindowHandle win $winId
    win SetClientType "HyperView"
    win GetClientHandle client
} {
    win SetClientType "HV"
    win GetClientHandle client
}]]
if {![lindex $r 0]} {
    export_log "FATAL: session handle chain failed (both HyperView/HV client types)"
    set FAILED 1
} else {
    set client client
}

# Measure 数值格式：科学计数 + 10 位（GUI formatandprec 同款）。
# 默认 Fixed/3 位会把 GetValueList 的值截成 3 位小数（实测 0.00205771019682 → 0.002）
if {!$FAILED} {
    catch { client SetMeasureNumericFormat scientific }
    catch { client SetMeasureNumericPrecision 10 }
    export_log "measure numeric format: scientific precision 10"
}

# ---- 2) 打开 H3D（v4 真机定案：AddModel 是 2022 唯一打开路径，poIPost 没有
#         OpenFile；OpenFile 两形式仅留作旧版兜底）
if {!$FAILED} {
    dump_methods $client "client(poIPost)"
    set opened [lindex [try_forms "open h3d" [list \
        {$client AddModel $H3D_IN} \
        {$client OpenFile $H3D_IN} \
        {$client OpenFile "H3D" $H3D_IN}]] 0]
    if {$opened} {
        catch { $client WaitForResults }
    } else {
        export_log "FATAL: cannot open h3d"
        set FAILED 1
    }
}

# ---- 3) 模型句柄（官方形式：GetActiveModel + GetModelHandle model $index）
if {!$FAILED} {
    if {![acquire "get model handle" [list \
        {set modelIndex [client GetActiveModel]; client GetModelHandle model $modelIndex} \
        {client GetModelHandle model 1}] model]} {
        export_log "FATAL: no model handle"
        set FAILED 1
    }
}
if {!$FAILED} { dump_methods $model "model(poIModel)" }

# ---- 4) 写 .fem：查询字段为官方 Note 示例同款（node.id/node.coords、element.id/
#         element.config/element.connectivity），行形态自适应嵌套列表/平铺/逗号串
set nodeIds {}
set elemIds {}
set nodeCount 0
set elemCount 0
set g_elemConn [dict create]
set poolNode "node"
set poolComp "solid"
set compId 1
if {!$FAILED} {
    if {[catch {
        set femCh [open $OUT_FEM w]
    } msg]} {
        export_log "FATAL: cannot create $OUT_FEM: $msg"
        set FAILED 1
    } else {
        puts $femCh "BEGIN BULK"
        if {[catch {
            set hasQ [acquire "query ctrl handle" [list \
                {$model GetQueryCtrlHandle q} \
                {client GetQueryCtrl q}] q]
            if {!$hasQ} {
                export_log "WARN: no query ctrl, .fem only headers"
            } else {
                dump_methods $q "q(poIQueryCtrl)"
                init_selection_sets $model

                # 4a) 节点坐标 → GRID（迭代器用唯一名，避免同名重复注册）
                bind_ss $q node
                if {![catch {$q SetQuery "node.id node.coords"} msgN]} {
                    set itName "iter[incr iterSeq]"
                    if {[acquire "node iterator" [list "\$q GetIteratorHandle $itName" {$q GetIteratorHandle}] $itName]} {
                        set iter $itName
                        set bad 0
                        set logged 0
                        while {[$iter Valid]} {
                            set row [$iter GetDataList]
                            if {!$logged} {
                                export_log "first node row: <$row>"
                                set logged 1
                            }
                            set nid ""; set x ""; set y ""; set z ""
                            if {[llength $row] >= 4} {
                                set nid [lindex $row 0]
                                set x [lindex $row 1]; set y [lindex $row 2]; set z [lindex $row 3]
                            } elseif {[llength $row] == 2} {
                                set nid [lindex $row 0]
                                set c [lindex $row 1]
                                if {[llength $c] == 3} {
                                    set x [lindex $c 0]; set y [lindex $c 1]; set z [lindex $c 2]
                                } else {
                                    set parts [split $c ,]
                                    if {[llength $parts] >= 3} {
                                        set x [string trim [lindex $parts 0]]
                                        set y [string trim [lindex $parts 1]]
                                        set z [string trim [lindex $parts 2]]
                                    }
                                }
                            }
                            if {$nid ne "" && [string is integer -strict $nid] && [string is double -strict $x] && [string is double -strict $y] && [string is double -strict $z]} {
                                puts $femCh "GRID,$nid,,[fmt10 $x],[fmt10 $y],[fmt10 $z]"
                                lappend nodeIds $nid
                                incr nodeCount
                            } else {
                                incr bad
                            }
                            $iter Next
                        }
                        catch { $iter ReleaseHandle }
                        export_log "fem nodes written=$nodeCount unparsed=$bad"
                        # v7.7 GUI 定案：pool 名（Measure AddNode 需要）
                        set itName "iter[incr iterSeq]"
                        if {![catch {$q SetQuery "node.pool node.id"} msgP]} {
                            if {![catch {$q GetIteratorHandle $itName} m2]} {
                                if {[$itName Valid]} {
                                    set poolNode [lindex [$itName GetDataList] 0]
                                }
                                catch { $itName ReleaseHandle }
                            }
                        }
                        export_log "node pool: $poolNode"
                    }
                } else {
                    export_log "WARN: node query rejected: $msgN"
                }

                # 4b) 单元连接 → CTETRA/CHEXA/CQUAD4/...（不支持构型计数跳过）
                bind_ss $q element
                if {![catch {$q SetQuery "element.id element.config element.connectivity"} msgE]} {
                    set itName "iter[incr iterSeq]"
                    if {[acquire "element iterator" [list "\$q GetIteratorHandle $itName" {$q GetIteratorHandle}] $itName]} {
                        set iter $itName
                        array unset unknownCfg
                        set bad 0
                        set logged 0
                        while {[$iter Valid]} {
                            set row [$iter GetDataList]
                            if {!$logged} {
                                export_log "first element row: <$row>"
                                set logged 1
                            }
                            if {[llength $row] >= 3} {
                                set eid [lindex $row 0]
                                set cfg [lindex $row 1]
                                set conn [lindex $row 2]
                                if {[llength $conn] == 1 && [string first "," $conn] >= 0} {
                                    set conn [split [string map {, " "} $conn]]
                                }
                                set card [elem_card $cfg]
                                if {[llength $card] == 0 && [string is integer -strict $cfg]} {
                                    # 未知数字码：按连接数推断构型（记录日志）
                                    switch -- [llength $conn] {
                                        8  { set card [list CHEXA 8] }
                                        20 { set card [list CHEXA 20] }
                                        10 { set card [list CTETRA 10] }
                                        6  { set card [list CPENTA 6] }
                                        5  { set card [list CPYRAM 5] }
                                        4  { set card [list CQUAD4 4] }
                                        3  { set card [list CTRIA3 3] }
                                        2  { set card [list CROD 2] }
                                    }
                                    if {[llength $card] == 2} {
                                        export_log "  config $cfg inferred as [lindex $card 0] (by conn count)"
                                    }
                                }
                                if {[llength $card] == 2 && [llength $conn] > [lindex $card 1]} {
                                    # 节点数超出卡型预期：截断，避免写出超长卡
                                    set conn [lrange $conn 0 [expr {[lindex $card 1] - 1}]]
                                }
                                if {[llength $card] == 2 && $eid ne "" && [string is integer -strict $eid] && [llength $conn] >= [lindex $card 1]} {
                                    set cname [lindex $card 0]
                                    set line "$cname,$eid,1"
                                    if {$cname eq "CBEAM"} {
                                        # H3D beam 第 3 节点是方向节点：文本卡改用默认方向矢量
                                        append line ",[lindex $conn 0],[lindex $conn 1],0.0,0.0,1.0"
                                    } else {
                                        foreach n $conn { append line ",$n" }
                                    }
                                    puts $femCh $line
                                    lappend elemIds $eid
                                    dict set g_elemConn $eid $conn
                                    incr elemCount
                                } elseif {[llength $card] == 0} {
                                    if {![info exists unknownCfg($cfg)]} { set unknownCfg($cfg) 0 }
                                    incr unknownCfg($cfg)
                                } else {
                                    incr bad
                                }
                            } else {
                                incr bad
                            }
                            $iter Next
                        }
                        catch { $iter ReleaseHandle }
                        export_log "fem elements written=$elemCount unparsed=$bad unsupportedConfigs=[array names unknownCfg]"
                        # v7.7 GUI 定案：pool/compId（Measure AddNode 需要）+ 节点测量三角映射
                        set itName "iter[incr iterSeq]"
                        if {![catch {$q SetQuery "component.pool component.id"} msgP]} {
                            if {![catch {$q GetIteratorHandle $itName} m2]} {
                                if {[$itName Valid]} {
                                    set row [$itName GetDataList]
                                    set poolComp [lindex $row 0]
                                    set compId [lindex $row 1]
                                }
                                catch { $itName ReleaseHandle }
                            }
                        }
                        export_log "component pool: $poolComp compId: $compId"
                        build_node_measure_map
                    }
                } else {
                    export_log "WARN: element query rejected: $msgE"
                }
            }

            # 占位属性卡（导入端按属性 id 分组用；材料/厚度不是 h3d 携带信息）
            puts $femCh "MAT1,1,210000.0,,0.3,7.85e-9"
            puts $femCh "PSHELL,1,1,0.1"
            puts $femCh "PSOLID,1,1"
            puts $femCh "PROD,1,1"
            puts $femCh "PBEAM,1,1,0.01,0.01"
            puts $femCh "ENDDATA"
        } msg2]} {
            export_log "FATAL: fem iteration failed: $msg2"
        }
        catch { close $femCh }
    }
}

# ---- 5) 写 .hwascii：官方分组布局（组头 $BINDING/$COLUMN_INFO/$RESULT_TYPE + $TIME 块），
#         与 AgaWriter 产物同构；每组显式重发 $BINDING（官方默认 ELEMENT 陷阱）
set hwaGroups 0
set totalSteps 0
set nSkipTypes 0
if {!$FAILED} {
    if {[catch {
        set hwaCh [open $OUT_HWA w]
    } msg]} {
        export_log "FATAL: cannot create $OUT_HWA: $msg"
        set FAILED 1
    } else {
        puts $hwaCh "ALTAIR ASCII FILE"
        puts $hwaCh "\$SUBCASE = 1 \"MxSim H3D Import\""

        # v7.3 真机定案：SetResult 实现按【第一参数=文件名】解释（官方文档的
        # "类型在前"是错的）；1arg(file) 形式命中。挂库成功判据 =
        # GetResultFileName 非空（不信任 SetResult 返回值）。
        set srOK 0
        foreach srForm [list \
            {$model SetResult $H3D_IN} \
            {$model SetResult "UNKNOWN" $H3D_IN}] \
            srDesc [list "1arg(file)" "2arg(type,file)(legacy)"] {
            if {![catch {eval $srForm} msgSR]} {
                set srFN ""
                catch { set srFN [string trim [$model GetResultFileName]] }
                export_log "SetResult($srDesc): res='[string range $msgSR 0 40]' GetResultFileName='$srFN'"
                if {$srFN ne ""} {
                    set srOK 1
                    export_log "SetResult attach OK"
                    break
                }
            } else {
                export_log "SetResult($srDesc) FAIL: $msgSR"
            }
        }
        if {!$srOK} {
            export_log "WARN: 结果库未挂上(GetResultFileName 为空)，.hwascii 可能只有头"
        }

        set hasRc [acquire "result ctrl handle" [list \
            {$model GetResultCtrlHandle rc} \
            {$model GetResultCtrlHandle rc $modelIndex} \
            {client GetResultCtrl rc}] rc]
        if {!$hasRc} {
            export_log "WARN: no result ctrl, .hwascii header only"
        }
        set hasQ 1
        if {[info commands q] eq ""} {
            # fem 阶段已拿到 q 时不重复获取（避免句柄同名重复注册）
            set hasQ [acquire "query ctrl handle (results)" [list \
                {$model GetQueryCtrlHandle q} \
                {client GetQueryCtrl q}] q]
        }

        if {$hasRc && $hasQ} {
            dump_methods $rc "rc(poIResultCtrl)"

            # 子工况枚举（官方约定：subcase 从 1 起）
            set subcases {}
            set r [try_forms "subcase list" [list {set subcases [$rc GetSubcaseList]}]]
            if {![lindex $r 0] || [llength $subcases] == 0} {
                set nsc 0
                set r2 [try_forms "subcase count" [list {set nsc [$rc GetNumberOfSubcases]}]]
                if {[lindex $r2 0] && $nsc > 0} {
                    for {set i 1} {$i <= $nsc} {incr i} { lappend subcases $i }
                } else {
                    set subcases {1}
                }
            }
            export_log "subcases: $subcases"

            set cc ""
            set V [dict create]
            set MAXCELLS 8000000
            set MAXTOTAL 8000000

            foreach sid $subcases {
                set r [try_forms "set subcase $sid" [list \
                    {$rc SetCurrentSubcase $sid} \
                    {$rc SetSubcase $sid}]]
                catch { $client WaitForResults }

                set types {}
                set r [try_forms "datatype list $sid" [list \
                    {set types [$rc GetDataTypeList $sid]} \
                    {set types [$rc GetDataTypeList]} \
                    {set types [client GetDataTypeList]}]]
                if {![lindex $r 0]} { continue }
                export_log "subcase $sid types: $types"

                foreach label $types {
                    if {[catch {
                        set comps {}
                            set r [try_forms "component list '$label'" [list \
                                {set comps [$rc GetDataComponentList $sid $label]} \
                                {set comps [client GetDataComponentList $label]}]]
                            if {![lindex $r 0] || [llength $comps] == 0} {
                                # v7.x：GetDataComponentList 未在 2022 真机证实，按类型名给默认分量
                                set up [string toupper $label]
                                if {[string first "DISPLACEMENT" $up] >= 0} {
                                    set comps {X Y Z}
                                    export_log "component list fallback '$label' -> X Y Z"
                                } elseif {[string first "STRESS" $up] >= 0} {
                                    set comps {XX YY ZZ XY YZ ZX}
                                    export_log "component list fallback '$label' -> XX YY ZZ XY YZ ZX"
                                }
                            }
                            if {[llength $comps] == 0} {
                                export_log "SKIP '$label': no component list"
                                incr nSkipTypes
                            } else {
                            set isComplex 0
                            foreach c $comps {
                                if {[regexp {(M/P)|(R/I)} $c]} { set isComplex 1 }
                            }
                            set fmt ""
                            catch { set fmt [string tolower [$rc GetDataTypeFormat $sid $label]] }
                            if {[string first "complex" $fmt] >= 0} { set isComplex 1 }
                            if {$isComplex} {
                                export_log "SKIP '$label': complex results (M/P or R/I) not supported"
                                incr nSkipTypes
                            } else {
                                # 列选择：张量 6 列 / 矢量 3 列 / 标量 1 列（Mag/vonMises
                                # 等派生列跳过，导入端自行计算）；列序按官方 XX..ZX/X Y Z 重排
                                set up {}
                                foreach c $comps { lappend up [string toupper [string trim $c]] }
                                set cols {}
                                set kind ""
                                if {[lsearch -exact $up "XX"] >= 0 || [string first "tensor" $fmt] >= 0} {
                                    set kind tensor
                                    foreach want {XX YY ZZ XY YZ ZX} {
                                        set ix [lsearch -exact $up $want]
                                        if {$ix >= 0} { lappend cols [list [lindex $comps $ix] $want] }
                                    }
                                }
                                if {[llength $cols] == 0 && ([lsearch -exact $up "X"] >= 0 || [string first "vector" $fmt] >= 0)} {
                                    set kind vector
                                    foreach want {X Y Z} {
                                        set ix [lsearch -exact $up $want]
                                        if {$ix >= 0} { lappend cols [list [lindex $comps $ix] $want] }
                                    }
                                }
                                if {[llength $cols] == 0} {
                                    set kind scalar
                                    lappend cols [list [lindex $comps 0] ""]
                                }
                                # 列数必须与后缀严格匹配（导入端按 (s)=1/(v)=3/(t)=6 列解析）：
                                # 部分分量的张量/矢量整组跳过，防止写出列数不符的坏组
                                set skipIncomplete 0
                                if {$kind eq "tensor" && [llength $cols] != 6} { set skipIncomplete 1 }
                                if {$kind eq "vector" && [llength $cols] != 3} { set skipIncomplete 1 }

                                set binding ""
                                set bindingKnown 0
                                catch {
                                    set binding [string tolower [$rc GetDataTypeBinding $sid $label]]
                                    set bindingKnown 1
                                }
                                if {$skipIncomplete} {
                                    export_log "SKIP '$label': incomplete $kind components ([llength $cols] cols), partial columns not supported"
                                    incr nSkipTypes
                                } elseif {[string first "node_on_element" $binding] >= 0 || [string first "nodeonelement" $binding] >= 0} {
                                    export_log "SKIP '$label': node_on_element binding not supported"
                                    incr nSkipTypes
                                } else {
                                    set elemental 0
                                    if {[string first "element" $binding] >= 0} { set elemental 1 }

                                    set nsim 1
                                    set r [try_forms "sim count '$label'" [list {set nsim [$rc GetNumberOfSimulations $sid]}]]
                                    if {![lindex $r 0] || $nsim < 1} { set nsim 1 }

                                    if {[info commands cc] eq ""} {
                                        # v7.3 真机：model/client 归属均 method-not-found，
                                        # rc 归属（探针 v7.5 验证中）排第一
                                        set hasCc [acquire "contour ctrl handle" [list \
                                            {$rc GetContourCtrlHandle cc} \
                                            {$model GetContourCtrlHandle cc} \
                                            {rc GetContourCtrl cc} \
                                            {client GetContourCtrl cc}] cc]
                                        if {$hasCc} { dump_methods $cc "cc(poIContourCtrl)" }
                                    }
                                    if {[info commands cc] eq ""} {
                                        export_log "SKIP '$label': no contour ctrl"
                                        incr nSkipTypes
                                    } else {
                                        set setOk [lindex [try_forms "set result type '$label'" [list \
                                            {$cc SetDataType $label} \
                                            {$cc SetResultType $label} \
                                            {$cc SetResultDef $label} \
                                            {$rc SetResultType $sid $label}]] 0]
                                        if {!$setOk} {
                                            export_log "SKIP '$label': cannot set contour result type"
                                            incr nSkipTypes
                                        } else {
                                            # v7.7 GUI 定案（postDisCommand::prcDisplayMinMax）：
                                            # 云图需 SetEnableState true 才激活；Apply 后值才落地。
                                            catch { $cc SetEnableState true }
                                            # animator/legend 句柄名每次唯一（重名注册会失败）
                                            set anH "anH_[incr ::iterSeq]"
                                            # v7.7 GUI 定案（postGetHandles.tcl prcGetAnimatorHandle）：
                                            # animator 句柄在 PAGE 上：page GetAnimatorHandle，
                                            # client 上没有此方法（真机 FAILED 实测）
                                            if {![catch { page GetAnimatorHandle $anH } msgA]} {
                                                set anName $anH
                                                export_log "animator handle OK: page GetAnimatorHandle -> $anH"
                                            } else {
                                                set anName ""
                                                export_log "animator handle FAIL: [string range $msgA 0 70]"
                                            }
                                            # 图例句柄：GetMin/GetMax 验证云图值已计算
                                            set lgH "lgH_[incr ::iterSeq]"
                                            acquire "legend handle" [list \
                                                {$cc GetLegendHandle $lgH} \
                                                {client GetLegendCtrlHandle $lgH} \
                                                {$model GetLegendCtrlHandle $lgH} \
                                                {$rc GetLegendCtrlHandle $lgH}] $lgH
                                            set lgName ""
                                            if {[info commands $lgH] ne ""} { set lgName $lgH }
                                            if {$lgName ne ""} {
                                                set lmin ""
                                                set lmax ""
                                                catch { set lmin [$lgName GetMin] }
                                                catch { set lmax [$lgName GetMax] }
                                                export_log "legend min/max: $lmin / $lmax"
                                            } else {
                                                export_log "legend handle FAILED"
                                            }
                                            set ids $nodeIds
                                            if {$elemental} { set ids $elemIds }
                                            if {[llength $ids] == 0} {
                                                export_log "SKIP '$label': no canonical ids for binding"
                                                incr nSkipTypes
                                            } else {
                                                set cells [expr {$nsim * [llength $cols] * [llength $ids]}]
                                                if {$cells > $MAXCELLS || [dict size $V] + $cells > $MAXTOTAL} {
                                                    export_log "SKIP '$label': too large (cells=$cells, cached=[dict size $V], caps $MAXCELLS/$MAXTOTAL), raise caps to export"
                                                    incr nSkipTypes
                                                } else {
                                                    set simTimes {}
                                                    set simLabels {}
                                                    set colMissing 0
                                                    # v7.5 真机定案：GetSimulationList <sid> 返回值式给步标签
                                                    set r [try_forms "sim list '$label'" [list {set simLabels [$rc GetSimulationList $sid]}]]
                                                    for {set si 0} {$si < $nsim} {incr si} {
                                                        # simulation 下标从 0 起（官方约定）
                                                        set setSimOk [lindex [try_forms "set simulation '$label'#$si" [list \
                                                            {$rc SetCurrentSimulation $si} \
                                                            {$cc SetSimulation $si} \
                                                            {$cc SetStep $si} \
                                                            {$cc SetSimulation $label $si}]] 0]
                                                        if {!$setSimOk} {
                                                            lappend simTimes ""
                                                            continue
                                                        }
                                                        set t $si
                                                        set lbl ""
                                                        if {$si < [llength $simLabels]} { set lbl [string trim [lindex $simLabels $si]] }
                                                        if {$lbl eq ""} {
                                                            set r [try_forms "sim label '$label'#$si" [list \
                                                                {set lbl [$rc GetSimulationLabel $sid $si]} \
                                                                {set lbl [$rc GetSimulationLabel $si]}]]
                                                        }
                                                        if {$lbl ne ""} {
                                                            set nums [regexp -all -inline -- {-?[0-9]+\.?[0-9]*(?:[eE][+-]?[0-9]+)?} $lbl]
                                                            if {[llength $nums] > 0} { set t [lindex $nums end] }
                                                        }
                                                        lappend simTimes $t

                                                        foreach cinfo $cols {
                                                            # 每列重算前缀：绑定自动切换后 elemental 可能已翻转
                                                            set prefix "node"
                                                            if {$elemental} { set prefix "element" }
                                                            set cname [string trim [lindex $cinfo 0]]
                                                            # v7.7 GUI 源码定案（postReaCommand.tcl 172-196）：
                                                            # SetDataComponent 为 <pool> <component> 双参，
                                                            # 单参会被静默忽略（云图无分量 → 无值）。
                                                            # 节点结果 pool=node；其余按 GUI 习惯
                                                            # shell/solid/line 全设。
                                                            if {$elemental} {
                                                                foreach pool [list shell solid line] {
                                                                    catch { $cc SetDataComponent $pool $cname }
                                                                }
                                                                set setC 1
                                                            } else {
                                                                set setC [lindex [try_forms "set component '$cname'" [list \
                                                                    {$cc SetDataComponent node $cname} \
                                                                    {$cc SetDataComponent $cname} \
                                                                    {$cc SetComponent $cname}]] 0]
                                                            }
                                                            if {!$setC} {
                                                                export_log "  WARN: cannot set component '$cname' of '$label'"
                                                                continue
                                                            }
                                                            # GUI 同款 Apply：重设步 + 显示选项 + Draw（值落地触发）
                                                            hv_apply_contour $anName rc $si
                                                            set key "${label}|${si}|${cname}"
                                                            # 主路径：Measure 官方批量读值（v7.7 GUI 源码定案）
                                                            lassign [collect_via_measure $client $modelIndex $prefix $key $poolComp $compId $poolNode $ids] n mrows
                                                            foreach mrow $mrows {
                                                                dict set V $key [lindex $mrow 0] [lindex $mrow 1]
                                                            }
                                                            set n [llength $mrows]
                                                            if {$n == 0} {
                                                                # 兜底：contour 查询（含诊断电池）
                                                                set n [collect_values $q $prefix $key V]
                                                            }
                                                            if {$n == 0 && !$bindingKnown} {
                                                                set alt "element"
                                                                if {$elemental} { set alt "node" }
                                                                set n [collect_values $q $alt $key V]
                                                                if {$n > 0} {
                                                                    set elemental [expr {1 - $elemental}]
                                                                    set ids $nodeIds
                                                                    if {$elemental} { set ids $elemIds }
                                                                    export_log "  binding auto-switched to $alt for '$label'"
                                                                }
                                                            }
                                                            if {$n > 0 && $n < [llength $ids]} {
                                                                # 行数少于实体数才是可疑信号（值缺失）；多出的行
                                                                # 属正常（被跳过构型的实体有值但不写出）
                                                                export_log "  NOTE '$cname' rows=$n < ids=[llength $ids] (missing values, selection set?)"
                                                            }
                                                            if {$n < 0} { incr colMissing }
                                                        }
                                                    }

                                                    # 组写出（每步仅在取到数据时输出 $TIME 块）
                                                    set suffix "s"
                                                    if {$kind eq "vector"} { set suffix "v" }
                                                    if {$kind eq "tensor"} { set suffix "t" }
                                                    puts $hwaCh "\$BINDING = [expr {$elemental ? {ELEMENT} : {NODE}}]"
                                                    puts $hwaCh "\$COLUMN_INFO = ENTITY_ID"
                                                    set head "\$RESULT_TYPE = ${label}($suffix)"
                                                    if {$kind eq "vector" && !$elemental} { append head " Anim Source" }
                                                    puts $hwaCh $head

                                                    set stepsWritten 0
                                                    set badVals 0
                                                    for {set si 0} {$si < $nsim} {incr si} {
                                                        set hasData 0
                                                        foreach cinfo $cols {
                                                            if {[dict exists $V "${label}|${si}|[string trim [lindex $cinfo 0]]"]} { set hasData 1 }
                                                        }
                                                        if {!$hasData} { continue }
                                                        puts $hwaCh "\$TIME = [fmt10 [lindex $simTimes $si]] sec"
                                                        foreach id $ids {
                                                            set line "$id"
                                                            foreach cinfo $cols {
                                                                set k "${label}|${si}|[string trim [lindex $cinfo 0]]"
                                                                if {[dict exists $V $k $id]} {
                                                                    set v [dict get $V $k $id]
                                                                    if {![string is double -strict $v]} { set v 0; incr badVals }
                                                                    append line " [fmt10 $v]"
                                                                } else {
                                                                    append line " 0"
                                                                }
                                                            }
                                                            puts $hwaCh $line
                                                        }
                                                        incr stepsWritten
                                                    }
                                                    incr hwaGroups
                                                    incr totalSteps $stepsWritten
                                                    export_log "group '$label': kind=$kind binding=[expr {$elemental ? {ELEMENT} : {NODE}}] cols=[llength $cols] steps=$stepsWritten/$nsim missingCols=$colMissing badVals=$badVals"
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } terr]} {
                        export_log "WARN: type '$label' failed: $terr"
                        incr nSkipTypes
                    }
                }
            }
        }
        close $hwaCh
    }
}

# ---- 6) 成功判定：两份输出都非空且有实质内容才写 EXPORT_OK
if {!$FAILED} {
    set femSize 0
    set hwaSize 0
    catch { set femSize [file size $OUT_FEM] }
    catch { set hwaSize [file size $OUT_HWA] }
    # 成功判定：两份输出都非空、有网格、有组、且至少写出一步值
    # （totalSteps=0 = 只有分组头的空壳，批处理下值读取失败时会出现，不算成功）
    if {$femSize > 40 && $hwaSize > 40 && $nodeCount > 0 && $elemCount > 0 && $hwaGroups > 0 && $totalSteps > 0} {
        export_log "EXPORT_OK fem=$femSize hwa=$hwaSize nodes=$nodeCount elems=$elemCount groups=$hwaGroups steps=$totalSteps"
    } else {
        export_log "EXPORT_INCOMPLETE fem=$femSize hwa=$hwaSize nodes=$nodeCount elems=$elemCount groups=$hwaGroups steps=$totalSteps skippedTypes=$nSkipTypes"
        set FAILED 1
    }
}

if {$LOG_CH ne ""} { close $LOG_CH }
catch { hwi CloseStack }
# 只在批处理（hw_* -b 会设 HW_BATCH_MODE）下退出；GUI 控制台 source 本脚本
# 作为备份通道时不关 GUI（门禁以日志 EXPORT_OK/INCOMPLETE 为准，不依赖退出码）
if {[info exists ::env(HW_BATCH_MODE)]} {
    if {$FAILED} { exit 1 }
    exit 0
}
