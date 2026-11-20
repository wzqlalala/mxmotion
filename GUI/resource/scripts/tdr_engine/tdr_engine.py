#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""tdr_engine.py——模板化 DOCX 报告渲染引擎（运行期子进程主入口，M1~M6 全链）。

用法：<python> tdr_engine.py <container.rtdoc> <dataset.json> <output.docx>
                [--compare <src_docx> <report_dir>]
  --compare：试导出比对模式（T-10）——导出成功后自动产基准①② diff 报告+
  残留计数（基准③④⑤人工核对清单），报告落 report_dir；不影响退出码。

链路（03-技术路线 §4.4，全部行为以 03/04-第3轮 字面为准）：
  M1 容器校验：manifest SHA1 对账+双版本检查+输入上限（S5-3）+rules 三层校验
    （复用 tdrrules，含 expect×tables[] 交叉核对）+rels 闭包不变量校验
    （T-01-②）——失败 exit!=0+结构化错误行，不留半成品；
  M3 资产装载与定位：容器内存解包（全程只读，E9 幂等基础）+槽→宿主块定位
    （B4：容器模板块唯一命中语义）；
  M4 装配与结构变换（白名单=02 §2.2 L4）：文本槽[域硬边界+域外子跨度单 run
    合并+域指令区只清高亮零结构改动（C3-01 R-b）]/表格槽[行组模板+vMerge
    restart/continue 成组复制/成组删除（C3-03/H-02）]/循环章[instantiate_loop：
    6.3 死子节剔除、每工况完整章（U-05）、字面"工况一" literal_rewrite（跨 run
    匹配、域回避 R-d）、renumber K_i=5+i（H-06 显式公式；子节连续重排 K_i.1/
    .2/.3；结论章=N+6 following_shift）、书签唯一化+REF 指令同步改写、
    _GoBack 剥离（H-11）、题注域后子跨度隐式承接（R3-01）]/图片槽[三形态锚点
    +版心/格宽等比]/条件措辞[句内槽先于整句槽（C3-07）+default 分支整句替换]/
    OLE preserve；末段全篇清高亮收口（run rPr+段落标记 rPr 双处，T-22）；
  M5 打包与域处理：资产化部件逐字节落包+块序重组 document.xml+关系续号重建
    +[Content_Types].xml 与 _rels/.rels 重建（MSFT 扩展类型映射 T-01-④）
    +docPr id 全篇重算+媒体接力（A-06 参数化）+TOC 置 dirty（settings 注入
    updateFields；空数据导出豁免——R4 双态口径）+SEQ 域缓存按文档序重算/
    REF 缓存尾数联动（TOC 缓存 PAGEREF 豁免——C3-08）+收尾全 rels Target
    在包内复核（T-01-③）；
  M6 消息回传：stdout 单行 JSON 阶段消息 print 强制 flush（T-14/A-02），
    stderr=异常栈；退出码=成败唯一判据（T-16/A-05）；
  finish_atomic：<out>.tdr_tmp<pid> 临时件→os.replace 原子改名（无 pre-delete
    ——T-13/A-01）；失败文案映射 PermissionError→"文件可能正被打开"/
    ENOSPC→"磁盘空间不足"（C3-09/H-17 单列）/其余 OSError→"输出目录不可写
    或路径无效（含 >260 字符长路径）"。

工程约束：python 3.7 兼容（typing.Dict/List/Tuple；open 显式 encoding；
stdout 写 JSON-lines 强制 flush）。

二期（08/09 蓝图批次 A，2026-11-12）：①缺项策略覆盖键 FR-211/212——数据集
顶层 missing_policy_override（缺省/null/空串=一期语义；三枚举=tdrrules.
MISSING_POLICY_ENUM），Ctx 构造期读入、_slot_policy 统一解析序（override＞
槽级 missing_policy＞L5 全局，三处解析点共用；顺带消除图片槽两处无 L5 回退
漂移=D3-5 已裁定行为语义变化点），M1 契约校验（非法值=条目级错误，引擎与
tdr_validate 双消费端一次覆盖），生效告警恰 1 条（E2.6）；②R2-03 修复
FR-261——iter_paragraph_spans 混合块（表+表外段）顶层直系 w:p 并入枚举
（文档序并集，纯块逐字节一致）。
"""

import bisect
import hashlib
import io
import math
import posixpath
import json
import os
import re
import sys
import traceback
from typing import Any, Dict, List, Optional, Tuple
from xml.parsers import expat

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import tdroox   # noqa: E402
import tdrrules  # noqa: E402


def sha1_bytes(data):
    # type: (bytes) -> str
    return hashlib.sha1(data).hexdigest()


# 引擎内建输入上限硬下限（CA-08/S5-3：manifest 声明值与内建值取更严者——
# 篡改 manifest 解除防线不可行；CC-10：cases 数上限在此口径内执行）
ENGINE_INPUT_LIMITS = {
    "max_parts": 500,               # 部件数上限
    "max_part_bytes": 209715200,    # 单部件 200MB
    "max_total_bytes": 1073741824,  # 容器解压总体积 1GB
    "max_ratio": 500,               # 解压比率上限
}
# 数据集 cases 上限（CC-10：主链耗时随工况数超线性增长，GUI 120s 看门狗
# 在大 N 必杀；超限=可读拒载文案而非无界运行）
MAX_CASES = 200
# manifest.json 读取上限（限量读 manifest 自身的保护帽）
_MANIFEST_CAP = 8388608   # 8MB


def _effective_limits(manifest):
    # type: (Dict[str, Any]) -> Dict[str, int]
    """声明值 ∩ 内建硬下限（逐键取更严者；缺省/非法值回落内建——CA-08）"""
    declared = manifest.get("input_limits") or {}
    eff = {}
    for k, builtin in ENGINE_INPUT_LIMITS.items():
        try:
            dv = int(declared.get(k, builtin))
        except (TypeError, ValueError):
            dv = builtin
        if dv <= 0:
            dv = builtin
        eff[k] = min(dv, builtin)
    return eff


def _manifest_integrity_ok(manifest):
    # type: (Dict[str, Any]) -> Optional[bool]
    """manifest 自身完整性锚复核（CC-09）：digest 落盘时对"除锚外字段的
    规范化 JSON（sort_keys 紧凑形态）"计算 SHA1 存入 manifest_sha1；M1 重算
    比对。旧格式容器（无锚）返回 None（向后兼容跳过）；相符 True；不符 False。"""
    want = manifest.get("manifest_sha1")
    if not want:
        return None
    core = dict((k, v) for k, v in manifest.items() if k != "manifest_sha1")
    got = sha1_bytes(json.dumps(core, ensure_ascii=False,
                                sort_keys=True).encode("utf-8"))
    return got == want


# ---------------------------------------------------------------------------
# M6 消息回传（JSON-lines + 强制刷新，T-14/A-02）
# ---------------------------------------------------------------------------

def emit_stage(stage, pct, msg=""):
    # type: (str, int, str) -> None
    sys.stdout.write(json.dumps({"stage": stage, "pct": int(pct), "msg": msg},
                                ensure_ascii=False) + "\n")
    sys.stdout.flush()


def emit_failed(msg, errors=None):
    # type: (str, Optional[List[str]]) -> None
    payload = {"stage": "failed", "pct": 0, "msg": msg}
    if errors:
        payload["errors"] = errors
    sys.stdout.write(json.dumps(payload, ensure_ascii=False) + "\n")
    sys.stdout.flush()


class EngineError(Exception):
    def __init__(self, msg, errors=None):
        # type: (str, Optional[List[str]]) -> None
        super(EngineError, self).__init__(msg)
        self.msg = msg
        self.errors = errors or []


# ---------------------------------------------------------------------------
# M1 容器校验
# ---------------------------------------------------------------------------

class Container(object):
    """容器内存形态（全程只读）：manifest/rules/index/资产字节/块 XML 列表"""

    def __init__(self):
        self.manifest = {}       # type: Dict[str, Any]
        self.rules = {}          # type: Dict[str, Any]
        self.index = {}          # type: Dict[str, Any]
        self.store_bytes = {}    # type: Dict[str, bytes]
        self.blocks = []         # type: List[Dict[str, Any]]  # [{"id","xml","meta"}]
        self.store_to_source = {}   # type: Dict[str, str]


def _check_policy_override(dataset):
    # type: (Optional[Dict[str, Any]]) -> List[str]
    """数据集顶层覆盖键契约校验（二期 FR-211/212，08 §2.2/09 §4.2）：缺省/
    null=不覆盖；空串=按缺省处理（GUI 恒不发空串——CB-08 口径）；非字符串
    或不在三枚举=条目级错误（定位 missing_policy_override+原因，FR-13
    bullets 形态）。枚举复用 tdrrules.MISSING_POLICY_ENUM 不复制（与规则层
    槽级 missing_policy 同域三值）。"""
    if not isinstance(dataset, dict):
        return []
    ov = dataset.get("missing_policy_override")
    if ov is None:
        return []
    if not isinstance(ov, str):
        return [u"missing_policy_override 必须为字符串（实测 %s）：允许值 %s，"
                u"或缺省/null/空串=不覆盖"
                % (type(ov).__name__, " / ".join(tdrrules.MISSING_POLICY_ENUM))]
    if ov == "":
        return []
    if ov not in tdrrules.MISSING_POLICY_ENUM:
        return [u"missing_policy_override 非法值 %r：必须为 %s 之一，"
                u"或缺省/null/空串=不覆盖"
                % (ov, " / ".join(tdrrules.MISSING_POLICY_ENUM))]
    return []


def load_and_verify_container(path, dataset):
    # type: (str, Dict[str, Any]) -> Container
    """M1（失败抛 EngineError，不留半成品）。读包走两阶段限量路径（CA-05/
    CA-08）：先零解压中央目录预检（内建硬下限）→限量读 manifest→声明值∩
    内建值取更严者→（必要时二次预检）→逐条目限量流式读取——内存峰值不再
    先于检查发生，篡改 manifest 不能解除防线。"""
    if not os.path.isfile(path):
        raise EngineError(u"规则模板容器不存在：%s" % path)
    try:
        tdroox.zip_preflight(path, ENGINE_INPUT_LIMITS)   # ⓪ 内建硬下限零解压预检
        manifest_bytes = tdroox.read_entry_limited(path, "manifest.json", _MANIFEST_CAP)
    except tdroox.PackageLimitError as e:
        raise EngineError(u"%s（S5-3 拒载）" % e)
    except Exception as e:
        raise EngineError(u"容器损坏或非 rtdoc 格式（无法按 zip 解包），请重新提炼：%s" % e)
    con = Container()
    con.manifest = {}
    if manifest_bytes is not None:
        try:
            con.manifest = json.loads(manifest_bytes.decode("utf-8"))
        except Exception as e:
            raise EngineError(u"manifest.json 解析失败：%s" % e)
    # manifest 自身完整性锚（CC-09：旧格式容器无锚=跳过，向后兼容）
    anchor_ok = _manifest_integrity_ok(con.manifest)
    if anchor_ok is False:
        raise EngineError(u"manifest 完整性校验失败（manifest 被改动，请重新提炼）")
    limits = _effective_limits(con.manifest)
    try:
        if limits != ENGINE_INPUT_LIMITS:
            tdroox.zip_preflight(path, limits)   # 声明值更严时按声明口径再预检
        store_bytes = tdroox.read_package_limited(path, limits)
    except tdroox.PackageLimitError as e:
        raise EngineError(u"%s（S5-3 拒载）" % e)
    except Exception as e:
        raise EngineError(u"容器损坏或非 rtdoc 格式（无法按 zip 解包），请重新提炼：%s" % e)
    con.store_bytes = store_bytes
    for required in ("manifest.json", "rules/rules.json", "assets/content/index.json"):
        if required not in store_bytes:
            raise EngineError(u"容器缺少关键文件 %s（请重新提炼）" % required)
    # 双版本检查（NFR-6：大版本不符拒载）
    fmaj = str(con.manifest.get("format_version", "")).split(".")[0]
    if fmaj != tdrrules.FORMAT_VERSION.split(".")[0]:
        raise EngineError(u"格式版本不符（容器 %s，引擎支持 %s.x）：请升级程序或按新格式重新提炼"
                          % (con.manifest.get("format_version"), tdrrules.FORMAT_VERSION))
    rmaj = str(con.manifest.get("rules_format_version", "")).split(".")[0]
    if rmaj != tdrrules.RULES_FORMAT_VERSION.split(".")[0]:
        raise EngineError(u"规则格式版本不符（%s）：请重新提炼" % con.manifest.get("rules_format_version"))
    # 输入上限（S5-3）已在限量读包阶段按 min(声明, 内建) 双重预检执行——
    # 此处对读入结果做一次复核（防中央目录谎报 file_size 的兜底面）
    limits = _effective_limits(con.manifest)
    total = 0
    if len(store_bytes) > limits["max_parts"]:
        raise EngineError(u"容器部件数超输入上限（S5-3）")
    for _n, v in store_bytes.items():
        total += len(v)
        if len(v) > limits["max_part_bytes"]:
            raise EngineError(u"容器存在超单件体积上限的部件（S5-3）")
    if total > limits["max_total_bytes"]:
        raise EngineError(u"容器解压总体积超上限（S5-3）")
    # SHA1 逐部件对账（E5 运行期：不符即拒载；仅资产化部件——T-01-⑤）
    bad = []
    for p in con.manifest.get("parts", []):
        if p.get("disposition") != "asset":
            continue   # content_blocks/engine_rebuild 部件不按字节对账
        st, want = p.get("store"), p.get("sha1")
        data = store_bytes.get(st)
        if data is None:
            bad.append(p.get("source_part", st))
        elif sha1_bytes(data) != want:
            bad.append(p.get("source_part", st))
    if bad:
        raise EngineError(u"完整性校验失败（部件被改动，请重新提炼）：%s" % ", ".join(bad))
    # 规则层加载（JSON+注释剥离）与三层校验
    try:
        con.rules = tdrrules.load_rules(store_bytes["rules/rules.json"].decode("utf-8"))
    except ValueError as e:
        raise EngineError(u"规则层非法：%s" % e)
    try:
        con.index = json.loads(store_bytes["assets/content/index.json"].decode("utf-8"))
    except Exception as e:
        raise EngineError(u"块索引解析失败（请重新提炼）：%s" % e)
    # 二期 FR-212：数据集顶层覆盖键契约校验（引擎 main 与 tdr_validate 经 M1
    # 双消费端一次覆盖——09 §3）；引擎 main 侧形状预检已先行（_check_dataset_shape）
    ov_errors = _check_policy_override(dataset)
    if ov_errors:
        raise EngineError(u"数据集覆盖键校验失败 %d 条" % len(ov_errors), ov_errors)
    rule_errors = tdrrules.validate_rules(con.rules, con.index, dataset)
    hard = [e for e in rule_errors if not e.is_warn]
    if hard:
        raise EngineError(u"规则校验失败 %d 条" % len(hard), [str(e) for e in hard])
    # 块装载（与 index.blocks 一一对应）
    n_blocks = int(con.manifest.get("blocks", {}).get("count", 0))
    index_blocks = con.index.get("blocks", [])
    if len(index_blocks) != n_blocks:
        raise EngineError(u"块索引与 manifest 计数不一致（请重新提炼）")
    con.blocks = []
    for i in range(n_blocks):
        key = "assets/content/blk_%03d.xml" % i
        if key not in store_bytes:
            raise EngineError(u"内容块 %s 缺失（请重新提炼）" % key)
        con.blocks.append({"id": index_blocks[i].get("id", "blk_%03d" % i),
                           "xml": store_bytes[key].decode("utf-8"),
                           "meta": index_blocks[i]})
    for p in con.manifest.get("parts", []):
        con.store_to_source[p.get("store", "")] = p.get("source_part", "")
    _verify_rels_closure(con)
    return con


def _verify_rels_closure(con):
    # type: (Container) -> None
    """rels 闭包不变量（T-01-②）：凡被 rels 引用的部件必须在容器资产内存在
    （单侧弃用=错误）。检查对象=容器内**实际存在**的 rels 部件 + manifest.
    package_rels（包级）。CC-02：可选部件（如 htsy 特有的 customizations.
    xml.rels、精简包无 document.xml.rels）缺席≠错误——按实际存在驱动闭包
    检查，缺席即视为无该侧引用；一旦存在则其引用必须闭合。"""
    # 凡被 rels 引用的部件必须在容器内存在（按源部件名集合核对——容器 store
    # 名经 manifest 对账表映射回源名；document.xml 以内容块形态存在）
    store_bytes = con.store_bytes
    checks = [(s, "word/") for s in ("assets/rels/document.xml.rels",
                                     "assets/rels/customizations.xml.rels")
              if s in store_bytes]
    checks.append(("__package_rels__", ""))
    source_names = set(p.get("source_part") for p in con.manifest.get("parts", []))
    source_names.add("word/document.xml")
    problems = []
    for store, base in checks:
        if store == "__package_rels__":
            xml = con.manifest.get("package_rels", "")
        else:
            xml = store_bytes[store].decode("utf-8")
        for m in re.finditer(r'<Relationship\b([^>]*)/>', xml):
            attrs = m.group(1)
            if 'TargetMode="External"' in attrs:
                continue
            tm = re.search(r'\bTarget="([^"]+)"', attrs)
            if not tm:
                continue
            target = tm.group(1).replace("\\", "/").lstrip("/")
            full = target if (not base or target.startswith(base)) else base + target
            full = posixpath.normpath(full)   # ../customXml/x → customXml/x
            if full not in source_names:
                problems.append(u"%s -> %s" % (store, target))
    if problems:
        raise EngineError(u"rels 闭包校验失败（悬空引用，T-01-②）：%s"
                          % "；".join(problems[:5]))


# ---------------------------------------------------------------------------
# 数据集取值（键路径五作用域解析 + 缺项策略告警聚合 T-22）
# ---------------------------------------------------------------------------

class Ctx(object):
    def __init__(self, dataset, rules):
        # type: (Dict[str, Any], Dict[str, Any]) -> None
        self.dataset = dataset
        self.rules = rules
        # FR-211：数据集顶层覆盖键（非空字符串；枚举合法性 M1 已校验）——
        # 三处策略解析点经 _slot_policy 统一消费；生效=告警恰 1 条（E2.6）
        self.policy_override = str(dataset.get("missing_policy_override") or "")
        self.case_warnings = {}   # type: Dict[str, List[str]]
        self.warnings = []        # type: List[str]
        if self.policy_override:
            self.warn(u"缺项策略已由数据集覆盖（missing_policy_override=%s）"
                      % self.policy_override)

    def warn_case(self, case_name, key):
        # type: (str, str) -> None
        self.case_warnings.setdefault(case_name, []).append(key)

    def warn(self, msg):
        # type: (str) -> None
        self.warnings.append(msg)

    def missing_text(self):
        # type: () -> str
        return str((self.rules.get("exception_policy") or {}).get("missing_text", "-"))

    def resolve(self, bind, case_i=None):
        # type: (str, Optional[int]) -> Tuple[Optional[Any], str]
        """bind 键路径取值，返回 (value, owner_case)。实例键 case.* 经
        instance_binding 语义解析为 cases[case_i-1]；全局共享键白名单
        （materials.*/model_stats.*/manual_fields.*/meta.*）不参与改写（C3-05）。"""
        b = str(bind)
        ds = self.dataset
        parts = b.split(".")
        if b.startswith("case.") and case_i is not None:
            cases = ds.get("cases") or []
            if case_i - 1 >= len(cases):
                return None, ""
            node = cases[case_i - 1]
            owner = str(node.get("name") or (u"工况%d" % case_i))
            for seg in parts[1:]:
                if not isinstance(node, dict) or seg not in node:
                    return None, owner
                node = node[seg]
            return node, owner
        if b.startswith("materials."):
            leaf = parts[-1]
            arr = ds.get("materials") or []
            return [m.get(leaf) for m in arr], ""
        if b.startswith("image."):
            key = parts[1] if len(parts) > 1 else ""
            field = parts[2] if len(parts) > 2 else "path"
            for img in ds.get("images") or []:
                if img.get("key") == key:
                    return img.get(field), ""
            return None, ""
        if b.startswith(("model_stats.", "manual_fields.", "meta.",
                         "conclusion.", "custom.")):
            node = ds.get(parts[0])
            for seg in parts[1:]:
                if not isinstance(node, dict) or seg not in node:
                    return None, ""
                node = node[seg]
            return node, ""
        return None, ""


# ---------------------------------------------------------------------------
# 缺项策略统一解析序（FR-211/212：override > 槽级 > L5 全局，3 解析点共用）
# ---------------------------------------------------------------------------

def _slot_policy(slot, ctx):
    # type: (Dict[str, Any], Ctx) -> str
    """缺项策略统一解析序（08 §2.2/09 §4.2）：数据集 override（非空字符串，
    M1 已校验枚举）→ 槽级 missing_policy → L5 exception_policy.missing_data
    → "dash_and_warn"。三处解析点（_resolve_slot_value 文本/条件缺项、
    process_image_slot 缺图与图片校验失败两分支）共用本入口，消除三处重复
    判定的漂移；顺带消除旧图片槽两处无 L5 回退漂移（D3-5 已裁定：行为语义
    变化点=图片槽缺图策略跟随 L5 全局默认；htsy 图片槽草稿显式声明 dash，
    基线产物零变化——S6 行为语义变化点登记）。"""
    if ctx.policy_override:
        return ctx.policy_override
    slot_policy = slot.get("missing_policy")
    if slot_policy:
        return str(slot_policy)
    return str((ctx.rules.get("exception_policy") or {}).get("missing_data",
                                                             "dash_and_warn"))


# ---------------------------------------------------------------------------
# 块内段落遍历（para/表格逐格，与 digest 同构）
# ---------------------------------------------------------------------------

def iter_paragraph_spans(xml):
    # type: (str) -> List[Tuple[int, int]]
    """块内全部段落字节区间（相对块起点坐标，按文档序）。表格分支：表基址
    ra、行基址 ta、格基址 ca 必须全部累加（CA-01/CC-08：旧实现漏加 ra/ta——
    表格内任何文本槽/字面锚点系统性错位、静默不替换甚至按错位字节区间改写）。
    R2-03（FR-261）：混合块（表+表外段）——顶层直系 w:p 与格内段落**并集按
    文档序**产出（旧实现含 w:tbl 即只枚举格内段落并 return，同块表外段不可达：
    occ="all" 漏替换、整数 occurrence 跨块计数少计——B1 并块 warn 拓扑）；
    纯块（htsy 77 块全纯）枚举序与旧实现逐字节一致=回归硬门槛。"""
    out = []   # type: List[Tuple[int, int]]
    if not (xml.lstrip().startswith("<w:tbl") or "<w:tbl>" in xml):
        for pa, pb in tdroox.child_spans(xml, "w:p"):
            out.append((pa, pb))
        return out
    tbl_spans = tdroox.child_spans(xml, "w:tbl")
    for ra, rb in tbl_spans:
        tbl = xml[ra:rb]
        for ta, tb in tdroox.child_spans(tbl, "w:tr"):
            row = tbl[ta:tb]
            for ca, cb in tdroox.child_spans(row, "w:tc"):
                cell = row[ca:cb]
                for pa, pb in tdroox.child_spans(cell, "w:p"):
                    out.append((ra + ta + ca + pa, ra + ta + ca + pb))
    # R2-03：补枚举顶层直系 w:p（落在任何 w:tbl 跨度内的=格内段，已随上方产出）
    for pa, pb in tdroox.child_spans(xml, "w:p"):
        if any(ra <= pa and pb <= rb for ra, rb in tbl_spans):
            continue
        out.append((pa, pb))
    out.sort(key=lambda s: s[0])   # 并集按文档序（CC-03 occurrence 全文档序语义）
    return out


def replace_sub(xml, start, end, new):
    # type: (str, int, int, str) -> str
    return xml[:start] + new + xml[end:]


# ---------------------------------------------------------------------------
# M4 文本槽（C3-01 R-b 域感知替换）
# ---------------------------------------------------------------------------

def _resolve_slot_value(slot, ctx, case_i, match):
    # type: (Dict[str, Any], Ctx, Optional[int], str) -> Tuple[Optional[str], str]
    """槽值解析+缺项策略（E1）：返回 (生效值或 None=保留原文不写, owner_case)。
    block_export=抛 EngineError；keep_highlight=告警后返回 None（不认领命中）；
    无值/dash=标缺值；有值=字符串值。"""
    bind = str(slot.get("bind", ""))
    value, owner = ctx.resolve(bind, case_i)
    if value is None:
        policy = _slot_policy(slot, ctx)   # FR-211 统一解析序（override>槽级>L5）
        if policy == "block_export":
            raise EngineError(u"槽 %r 绑定键 %r 无数据且策略=block_export" % (match, bind))
        if policy == "keep_highlight":
            if owner:
                ctx.warn_case(owner, bind)
            else:
                ctx.warn(u"键 %r 无数据（保留原文）" % bind)
            return None, owner
        value = ctx.missing_text()
        if owner:
            ctx.warn_case(owner, bind)
        else:
            ctx.warn(u"键 %r 无数据（标缺 %r）" % (bind, value))
        return value, owner
    return str(value), owner


def _plan_apply_text_group(xmls, slot_entries, phrase, ctx, case_i=None, bases=None):
    # type: (List[str], List[Tuple[int, Dict[str, Any]]], str, Ctx, Optional[int], Optional[Dict[int, int]]) -> Tuple[List[str], Dict[int, int], int]
    """BUG-07 计划-应用两阶段：共享锚点短语的一组 text 槽在**原文命中序列快照**
    上一次性分区，再统一套用——任何槽的序号解析不再受前序槽替换影响：
      计划：①整数槽（声明序）认领快照第 k 个命中（bases[si]=此前宿主块已消耗
            命中数，CC-03 跨块口径；整组一次规划时无需）；②"all"槽认领全部
            剩余命中；越界/序号被其他槽认领=显式告警+该槽不替换（绝不静默，
            C3-07/CC-11/E1）；
      应用：（块降序×段降序×段内降序）一次套用（BUG-06 共用核）——同块/同段
            多命中全部落字；包装边界守卫（CC-05）仅跳过该命中并告警。
    返回 (新 xmls, {slot_si: 实际替换数}, 命中总数)。"""
    hits = []   # type: List[Tuple[int, int, int, str, Any]]  # (xi, pa, pb, kind, payload)
    for xi, xml in enumerate(xmls):
        for pa, pb, kind, payload in _find_all_spans(xml, phrase):
            hits.append((xi, pa, pb, kind, payload))
    total = len(hits)
    bases = bases or {}
    claimed = set()      # type: set
    claims = {}          # type: Dict[int, List[int]]
    values = {}          # type: Dict[int, str]
    # ① 整数槽优先认领其序号位（声明序）
    for si, slot in slot_entries:
        occ = slot.get("anchor", {}).get("occurrence", 1)
        if occ == "all":
            continue
        try:
            k = int(occ)
        except (TypeError, ValueError):
            k = 1
        value, _owner = _resolve_slot_value(slot, ctx, case_i, phrase)
        claims[si] = []
        if value is None:
            continue   # keep_highlight/无值=保留原文（缺项告警已发，不认领不告失配）
        values[si] = value
        local = k - 1 - bases.get(si, 0)
        if local < 0:
            continue   # 该 occurrence 已在此前宿主块满足（CC-03 跨块计数）
        if local >= total:
            ctx.warn(u"槽 %r occurrence=%s 命中 %d 处（越界失配，原样保留）"
                     % (phrase, occ, total + bases.get(si, 0)))
            continue
        if local in claimed:
            ctx.warn(u"槽 %r occurrence=%s 第 %d 处已被同锚点其他槽认领（冲突，本槽原样保留）"
                     % (phrase, occ, k))
            continue
        claims[si].append(local)
        claimed.add(local)
    # ② "all" 槽认领剩余全部（首个 all 生效；重复 all/无剩余=显式告警）
    all_taken = False
    for si, slot in slot_entries:
        occ = slot.get("anchor", {}).get("occurrence", 1)
        if occ != "all":
            continue
        value, _owner = _resolve_slot_value(slot, ctx, case_i, phrase)
        claims[si] = []
        if value is None:
            continue
        values[si] = value
        if all_taken:
            ctx.warn(u"槽 %r occurrence=\"all\" 已被同锚点其他 all 槽认领（冲突，本槽原样保留）"
                     % phrase)
            continue
        all_taken = True
        if total == 0:
            ctx.warn(u"槽 %r 全文档未命中任何可替换跨度（原样保留）" % phrase)
            continue
        rest = [i for i in range(total) if i not in claimed]
        if not rest:
            ctx.warn(u"槽 %r occurrence=\"all\" 无剩余可认领命中（全部被整数槽分区，本槽原样保留）"
                     % phrase)
            continue
        claims[si] = rest
        claimed.update(rest)
    # ③ 应用：块降序×段降序×段内降序（命中集合基于原文快照，坐标互不失效）
    applied = dict((si, 0) for si, _s in slot_entries)
    groups = {}   # type: Dict[Tuple[int, int, int], List[Tuple[int, str, Any]]]
    for hi, (xi, pa, pb, kind, payload) in enumerate(hits):
        groups.setdefault((xi, pa, pb), []).append((hi, kind, payload))
    for (xi, pa, pb), items in reversed(list(groups.items())):
        para_xml = xmls[xi][pa:pb]
        changed = False
        for hi, kind, payload in reversed(items):
            owner_si = None
            for si, lst in claims.items():
                if hi in lst:
                    owner_si = si
                    break
            if owner_si is None:
                continue   # 未被任何槽认领（无 all 槽且整数未覆盖=保留原文）
            para_xml, used = _replace_span_in_para(para_xml, kind, payload,
                                                   phrase, values[owner_si], ctx)
            if used:
                applied[owner_si] += 1
                changed = True
        if changed:
            xmls[xi] = xmls[xi][:pa] + para_xml + xmls[xi][pb:]
    return xmls, applied, total


def replace_text_slot(block_xml, slot, ctx, case_i=None, occ_offset=0):
    # type: (str, Dict[str, Any], Ctx, Optional[int], int) -> Tuple[str, int]
    """文本槽替换单槽入口（BUG-07：与共享锚点组共用计划-应用两阶段核——
    原文命中快照解析，序号不受本槽替换影响）。occurrence 语义：整数 k=
    扣除 occ_offset（此前宿主块已消耗命中数，CC-03 跨块口径）后的原文命中
    序第 k 处；"all"=全部剩余命中。返回 (新块 XML, 本块消耗命中数——供跨块
    occ_offset 累计)。"""
    match = str(slot.get("anchor", {}).get("match", ""))
    occ = slot.get("anchor", {}).get("occurrence", 1)
    new_xmls, applied, total = _plan_apply_text_group(
        [block_xml], [(0, slot)], match, ctx, case_i, {0: occ_offset})
    consumed = applied.get(0, 0)
    if consumed == 0 and occ != "all":
        # 跨块累计口径：本块命中数<序号需求时把本块全部命中计入偏移
        try:
            k = int(occ)
        except (TypeError, ValueError):
            k = 1
        consumed = min(max(k - occ_offset, 0), total)
    return new_xmls[0], consumed


def _replace_span_in_para(para_xml, kind, payload, match, value, ctx):
    # type: (str, str, Any, str, str, Ctx) -> Tuple[str, int]
    """段落内单命中替换（BUG-06 共用核：块级与 all 批量应用同走此核）；
    B3 合并被包装边界守卫拒绝（CC-05）时告警并返回原样（消耗 0）。"""
    runs = tdroox.iter_runs(para_xml)
    run_spans = [(r.start, r.end) for r in runs]
    if kind == "hl":
        new_para = tdroox.replace_span_with_single_run(
            para_xml, run_spans, payload["run_range"][0], payload["run_range"][1], value)
        if new_para is None:
            ctx.warn(u"槽 %r 高亮跨度跨超链接/书签等包装边界，合并拒绝"
                     u"（原样保留，防结构损坏 CC-05）" % match)
            return para_xml, 0
    else:
        new_para = tdroox.rewrite_runs_with_text(para_xml, run_spans, payload, value)
    return new_para, 1


def _find_all_spans(block_xml, match):
    # type: (str, str) -> List[Tuple[int, int, str, Any]]
    """块内全部与 match 等值的可替换跨度（文档序；枚举序与 _find_span 一致：
    每段 hl 优先于字面）——occurrence="all" 批量替换的定位面（BUG-06：
    命中集合基于原文一次收集，自后向前应用）。返回 [(pa, pb, kind, payload)]。"""
    out = []   # type: List[Tuple[int, int, str, Any]]
    for pa, pb in iter_paragraph_spans(block_xml):
        para = block_xml[pa:pb]
        if match not in tdroox.fragment_text(para):
            continue
        runs = tdroox.iter_runs(para)
        for sp in tdroox.merge_highlight_spans(runs):
            if sp["text"] == match and not sp["in_field"] and not sp["in_field_result"]:
                out.append((pa, pb, "hl", sp))
        for hit in tdroox.match_literal_in_runs(runs, match):
            out.append((pa, pb, "lit", hit))
    return out


def _find_span(block_xml, match, nth=0):
    # type: (str, str, int) -> Optional[Tuple[str, int, int, Any]]
    """块内按文档序定位第 nth（0 基）个与 match 等值的可替换跨度：优先高亮
    跨度（域外子跨度），其次非高亮字面序列（match_literal_in_runs 仅非高亮
    run——R-d 同源；CC-03 与校验器 occurrence 全文档序语义一致）。"""
    seen = 0
    for pa, pb in iter_paragraph_spans(block_xml):
        para = block_xml[pa:pb]
        if match not in tdroox.fragment_text(para):
            continue
        runs = tdroox.iter_runs(para)
        for sp in tdroox.merge_highlight_spans(runs):
            if sp["text"] == match and not sp["in_field"] and not sp["in_field_result"]:
                if seen == nth:
                    return ("hl", pa, pb, sp)
                seen += 1
        for hit in tdroox.match_literal_in_runs(runs, match):
            if seen == nth:
                return ("lit", pa, pb, hit)
            seen += 1
    return None


# ---------------------------------------------------------------------------
# M4 表格槽（C3-03 行组+vMerge；T-09 表头策略）
# ---------------------------------------------------------------------------

def process_table_slot(block_xml, slot, ctx, case_i=None):
    # type: (str, Dict[str, Any], Ctx, Optional[int]) -> Tuple[str, bool]
    """表格槽：本块即目标表（宿主块映射保证）。行组模板=成组复制/成组删除
    （vMerge restart 保持、continue 链随组，不留孤悬挂）；单行模板=行复制。
    列绑定 keep_literal 保留原文；值缺失→标缺+告警。表头高亮清理由全篇
    highlight_clear 收口统一完成（T-22）。"""
    tbl_span = _find_table_span(block_xml)
    if tbl_span is None:
        ctx.warn(u"表格槽未在本块命中表格（原样保留）")
        return block_xml, False
    ta, tb = tbl_span
    tbl = block_xml[ta:tb]
    row_spans = tdroox.child_spans(tbl, "w:tr")
    columns = slot.get("columns", [])
    row_group = slot.get("row_group")
    need = _bound_row_count(slot, ctx, case_i)
    if row_group is not None:
        merge_col = int(row_group.get("merge_col", 0))
        restart_row = int(row_group.get("restart_row", 1))
        groups = _detect_row_groups(tbl, row_spans, merge_col, restart_row)
        if not groups:
            ctx.warn(u"表格槽 %r 行组检测未命中（vMerge/restart_row 不符，原样保留）"
                     % str(slot.get("anchor", {}).get("match", "")))
            return block_xml, False
        g0_first = row_spans[groups[0][0]][0]
        g_last = row_spans[groups[-1][1] - 1][1]
        head_xml = tbl[:g0_first]
        tail_xml = tbl[g_last:]
        group_spans = [(row_spans[g0][0], row_spans[g1 - 1][1]) for (g0, g1) in groups]
        keep = max(0, min(need, len(groups)))
        pieces = []
        for g in range(keep):
            ga, gb = group_spans[g]
            pieces.append(_bind_group_cells(tbl[ga:gb], columns, ctx, case_i, g,
                                            merge_col))
        if need > len(groups):
            ga, gb = group_spans[-1]
            template_group = tbl[ga:gb]
            for g in range(len(groups), need):
                pieces.append(_bind_group_cells(template_group, columns, ctx, case_i, g,
                                                merge_col))
        new_tbl = head_xml + "".join(pieces) + tail_xml
    else:
        data_rows = slot.get("data_rows") or []
        if not data_rows or len(row_spans) <= max(data_rows):
            ctx.warn(u"表格槽 %r 数据行声明无效（data_rows 越界或缺失，原样保留）"
                     % str(slot.get("anchor", {}).get("match", "")))
            return block_xml, False
        first_dr, last_dr = min(data_rows), max(data_rows)
        head_xml = tbl[:row_spans[first_dr][0]]
        tail_xml = tbl[row_spans[last_dr][1]:]
        template_row = tbl[row_spans[first_dr][0]:row_spans[last_dr][1]]
        pieces = []
        for _g in range(need):
            pieces.append(_bind_row_cells(template_row, columns, ctx, case_i))
        new_tbl = head_xml + "".join(pieces) + tail_xml
    return replace_sub(block_xml, ta, tb, new_tbl), True


def _bound_row_count(slot, ctx, case_i):
    # type: (Dict[str, Any], Ctx, Optional[int]) -> int
    for c in slot.get("columns", []) or []:
        b = str(c.get("bind", ""))
        v, _owner = ctx.resolve(b, case_i)
        if isinstance(v, list):
            return len(v)
    if any(str(c.get("bind", "")).startswith("case.") for c in slot.get("columns", []) or []):
        return 1   # 实例上下文：1 行
    return 1


def _detect_row_groups(tbl, row_spans, merge_col, restart_row):
    # type: (str, List[Tuple[int, int]], int, int) -> List[Tuple[int, int]]
    """行组检测：merge_col 单元格 vMerge=restart 开新组、continue 续组；
    restart_row 缺标记时按模板声明行号开组（C3-03 显式声明）。"""
    groups = []   # type: List[Tuple[int, int]]
    cur = None    # type: Optional[int]
    for ri, (ra, rb) in enumerate(row_spans):
        row = tbl[ra:rb]
        cells = tdroox.child_spans(row, "w:tc")
        vm = None
        if merge_col < len(cells):
            ca, cb = cells[merge_col]
            mm = re.search(r'<w:vMerge(?:\s+w:val="(\w+)")?\s*/>', row[ca:cb])
            vm = (mm.group(1) or "continue") if mm else None
        if ri == restart_row or vm == "restart":
            if cur is not None:
                groups.append((cur, ri))
            cur = ri
        elif vm is None:
            if cur is not None:
                groups.append((cur, ri))
                cur = None
    if cur is not None:
        groups.append((cur, len(row_spans)))
    return groups


def _bind_group_cells(grp, columns, ctx, case_i, g, merge_col):
    # type: (str, List[Dict[str, Any]], Ctx, Optional[int], int, int) -> str
    """行组各列绑定（materials 第 g 元素）：值写入组首行对应格（merge 列写
    restart 格；其余数据列写首行格、续行格保持模板原样）；keep_literal 保留。"""
    out = []
    pos = 0
    row_spans = tdroox.child_spans(grp, "w:tr")
    for ri, (ra, rb) in enumerate(row_spans):
        row = grp[ra:rb]
        if ri == 0:
            for c in columns:
                b = str(c.get("bind", ""))
                if b == "keep_literal":
                    continue
                arr, _owner = ctx.resolve(b, case_i)
                value = arr[g] if isinstance(arr, list) and g < len(arr) else None
                if value is None:
                    ctx.warn(u"键 %r 无数据（表格标缺）" % b)
                    value = ctx.missing_text()
                col = int(c.get("col", 0))
                cells = tdroox.child_spans(row, "w:tc")
                if col < len(cells):
                    ca, cb = cells[col]
                    row = replace_sub(row, ca, cb,
                                      _set_cell_text(row[ca:cb], str(value)))
        out.append(grp[pos:ra])
        out.append(row)
        pos = rb
    out.append(grp[pos:])
    return "".join(out)


def _bind_row_cells(row, columns, ctx, case_i):
    # type: (str, List[Dict[str, Any]], Ctx, Optional[int]) -> str
    """单行模板各格绑定（表7：case.* 实例键→实例值）。"""
    case = None
    if case_i is not None:
        cases = ctx.dataset.get("cases") or []
        if case_i - 1 < len(cases):
            case = cases[case_i - 1]
    out = []
    pos = 0
    for ci, (ca, cb) in enumerate(tdroox.child_spans(row, "w:tc")):
        cell = row[ca:cb]
        new_cell = cell
        for c in columns:
            if int(c.get("col", -1)) == ci:
                b = str(c.get("bind", ""))
                if b == "keep_literal":
                    break
                value = None
                if b.startswith("case.") and case is not None:
                    node = case
                    value = case
                    for seg in b.split(".")[1:]:
                        if not isinstance(node, dict) or seg not in node:
                            value = None
                            break
                        node = node[seg]
                    value = node
                else:
                    value, _owner = ctx.resolve(b, case_i)
                if value is None:
                    if case is not None:
                        ctx.warn_case(str(case.get("name") or ""), b)
                    else:
                        ctx.warn(u"键 %r 无数据（表格标缺）" % b)
                    value = ctx.missing_text()
                new_cell = _set_cell_text(cell, str(value))
                break
        out.append(row[pos:ca])
        out.append(new_cell)
        pos = cb
    out.append(row[pos:])
    return "".join(out)


def _set_cell_text(cell, value):
    # type: (str, str) -> str
    """单元格写值：首个 w:t 承载新值（保留其 run/rPr），其余 w:t 清空；
    无 w:t 的空格→向首个段落注入纯文本 run（格式随段落样式）。"""
    found = [False]

    def _sub_first(m):
        # type: (Any) -> str
        if found[0]:
            return m.group(1) + m.group(2)
        found[0] = True
        return m.group(1) + tdroox.xml_escape(value) + m.group(2)

    out = re.sub(r"(<w:t[^>]*>)[^<]*(</w:t>)", _sub_first, cell)
    if found[0]:
        return out
    for pa, pb in tdroox.child_spans(cell, "w:p"):
        at = cell.rfind("</w:p>", pa, pb)
        if at < 0:
            continue
        run = '<w:r><w:t xml:space="preserve">%s</w:t></w:r>' % tdroox.xml_escape(value)
        return cell[:at] + run + cell[at:]
    return out


def _find_table_span(block_xml):
    # type: (str) -> Optional[Tuple[int, int]]
    for ta, tb in tdroox.child_spans(block_xml, "w:tbl"):
        return (ta, tb)
    return None


# ---------------------------------------------------------------------------
# M4 图片槽（三形态锚点 + 版心/格宽等比，FR-24/E7；A-06 媒体接力）
# ---------------------------------------------------------------------------

def process_image_slot(block_xml, slot, ctx, case_i=None, registry=None,
                       box=(tdroox.DEFAULT_BODY_WIDTH_EMU, tdroox.DEFAULT_BODY_WIDTH_EMU * 3)):
    # type: (str, Dict[str, Any], Ctx, Optional[int], Optional[Any], Tuple[int, int]) -> Tuple[str, bool]
    """形态1=占位段落文本（match：替换占位字面为图）；形态2=空段落坐标
    （block_paragraph：块即目标空段）；形态3=表格坐标（table_match+cell：
    图入空单元格，caption_cell 子题注格不动——其高亮由收口清除）。缺图走
    missing_policy（形态1 dash=占位文本标缺；其余留空+告警）。图片文件先经
    魔数+可解析尺寸双校验（E7/BUG-02，C12-Q-03），不过=阻断该槽+区分文案
    告警，不落包不插 drawing。"""
    anchor = slot.get("anchor") or {}
    bind = str(slot.get("bind", ""))
    value, owner = ctx.resolve(bind, case_i)
    size_policy = (slot.get("size") or {}).get("policy", "fit_section_width")
    if value is None or not os.path.isfile(str(value)):
        policy = _slot_policy(slot, ctx)   # FR-211 统一解析序（override>槽级>L5）
        if policy == "block_export":
            raise EngineError(u"图片槽 %r 缺图且策略=block_export（%r）" % (bind, value))
        if owner:
            ctx.warn_case(owner, bind)
        else:
            # DX-07：内部字面 None 不直出（改用户说法）
            ctx.warn(u"图片槽 %r 无图（数据集未提供）" % bind
                     if value is None
                     else u"图片槽 %r 无图（%r）" % (bind, value))
        if policy == "dash_and_warn" and anchor.get("match"):
            return replace_text_slot(block_xml, slot, ctx, case_i)[0], False
        return block_xml, False
    path = str(value)
    # BUG-02/E7（C12-Q-03 口径）：魔数+可解析尺寸双校验（扩展名不作判据，
    # 首期 PNG/JPEG/BMP）；不过→阻断该槽+告警（文案区分），不落包不插
    # drawing，其余槽继续——绝不静默嵌入非图片字节
    fmt, dims = tdroox.probe_image(path)
    if fmt is None:
        reason = u"非图片字节"
    elif fmt not in ("png", "jpeg", "bmp"):
        reason = u"不支持的图片格式（%s，首期仅 PNG/JPEG/BMP）" % fmt
    elif dims is None:
        reason = u"图片格式（%s）头可识别但尺寸不可解析" % fmt
    else:
        reason = None
    if reason is not None:
        policy = _slot_policy(slot, ctx)   # FR-211 统一解析序（override>槽级>L5）
        if policy == "block_export":
            raise EngineError(u"图片槽 %r %s 且策略=block_export（%r）"
                              % (bind, reason, value))
        ctx.warn(u"图片槽 %r %s，已阻断该槽（不落包不插入），其余槽继续：%r"
                 % (bind, reason, value))
        if policy == "dash_and_warn" and anchor.get("match"):
            return replace_text_slot(block_xml, slot, ctx, case_i)[0], False
        return block_xml, False
    pw, ph = dims
    rid = registry.register(path)[0] if registry is not None else "rIdImgX"
    box_w, box_h = box
    if size_policy == "fit_cell":
        cell_span = _find_cell_span(block_xml, anchor)
        if cell_span is not None:
            cell = block_xml[cell_span[0]:cell_span[1]]
            tcw = re.search(r"<w:tcW\b[^>]*/>", cell)
            if tcw:
                wv = re.search(r'w:w="(\d+)"', tcw.group(0))
                wt = re.search(r'w:type="(\w+)"', tcw.group(0))
                # CA-16：w="0" 或 type="auto"（自适应列）不取格宽，回退版心宽
                if wv and int(wv.group(1)) > 0 and (not wt or wt.group(1) != "auto"):
                    box_w = int(wv.group(1)) * tdroox.TWIP_TO_EMU
                    box_h = box_w * 3
    cx, cy = tdroox.scaled_emu(pw, ph, box_w, box_h)
    pic = tdroox.picture_run_xml(rid, 0, cx, cy)   # docPr 占位，M5 全篇重算
    if anchor.get("element") == "table":
        cell_span = _find_cell_span(block_xml, anchor)
        if cell_span is None:
            ctx.warn(u"图片槽 %r 表格坐标未命中（%s）" % (bind, anchor.get("cell")))
            return block_xml, False
        ca, cb = cell_span
        cell = block_xml[ca:cb]
        pms = tdroox.child_spans(cell, "w:p")
        if not pms:
            return block_xml, False
        pa, pb = pms[0]
        para = cell[pa:pb]
        at = para.rfind("</w:p>")
        new_para = para[:at] + pic + para[at:]
        return replace_sub(block_xml, ca, cb, replace_sub(cell, pa, pb, new_para)), True
    if anchor.get("element") == "block_paragraph":
        at = block_xml.rfind("</w:p>")
        if at < 0:
            return block_xml, False
        return block_xml[:at] + pic + block_xml[at:], True
    # 形态 1：占位段落文本——占位字面清为空（保留 rPr）+图注入该段
    target = _find_span(block_xml, str(anchor.get("match", "")))
    if target is None:
        ctx.warn(u"图片槽 %r 占位字面未命中（%r）" % (bind, anchor.get("match")))
        return block_xml, False
    _kind, pa, pb, payload = target
    para = block_xml[pa:pb]
    runs = tdroox.iter_runs(para)
    run_spans = [(r.start, r.end) for r in runs]
    if _kind == "hl":
        para2 = tdroox.replace_span_with_single_run(
            para, run_spans, payload["run_range"][0], payload["run_range"][1], "")
        if para2 is None:
            ctx.warn(u"图片槽 %r 高亮跨度跨包装边界，合并拒绝（原样保留 CC-05）" % bind)
            return block_xml, False
    else:
        para2 = tdroox.rewrite_runs_with_text(para, run_spans, payload, "")
    at = para2.rfind("</w:p>")
    para2 = para2[:at] + pic + para2[at:]
    return replace_sub(block_xml, pa, pb, para2), True


def _find_cell_span(block_xml, anchor):
    # type: (str, Dict[str, Any]) -> Optional[Tuple[int, int]]
    phrase = str(anchor.get("table_match", ""))
    m = re.match(r"^r(\d+)c(\d+)$", str(anchor.get("cell", "")))
    if not m or not phrase:
        return None
    ri, ci = int(m.group(1)), int(m.group(2))
    for ta, tb in tdroox.child_spans(block_xml, "w:tbl"):
        tbl = block_xml[ta:tb]
        if phrase not in tdroox.fragment_text(tbl):
            continue
        rows = tdroox.child_spans(tbl, "w:tr")
        if ri >= len(rows):
            return None
        row = tbl[rows[ri][0]:rows[ri][1]]
        cells = tdroox.child_spans(row, "w:tc")
        if ci >= len(cells):
            return None
        return (ta + rows[ri][0] + cells[ci][0], ta + rows[ri][0] + cells[ci][1])
    return None


# ---------------------------------------------------------------------------
# M4 条件措辞槽（FR-26 + T-24/D-05 + C3-07 句内槽先于整句槽）
# ---------------------------------------------------------------------------

def render_conditional(block_xml, slot, ctx, case_i=None):
    # type: (str, Dict[str, Any], Ctx, Optional[int]) -> Tuple[str, bool]
    """判定键 null/无命中分支→整句替换为槽级 missing_text；命中分支→整句
    替换为分支文案（{manual_fields.project_subject} 等全限定键引用展开，
    H-14/C12-Q-01：裸键=校验期阻断，引擎严格作用域解析不做裸键回退）。"""
    phrase = str(slot.get("anchor", {}).get("match_prefix", ""))
    bind = str(slot.get("bind", ""))
    verdict, _owner = ctx.resolve(bind, case_i)
    text = None
    if verdict is not None:
        for br in slot.get("branches", []) or []:
            if _eval_when(str(br.get("when", "")), verdict):
                use = str(br.get("use", ""))
                branch_text, _o = ctx.resolve(use, case_i)
                if branch_text is None:
                    branch_text, _o = ctx.resolve("conclusion." + use, case_i)
                if isinstance(branch_text, str):
                    text = branch_text
                    break
    if text is None:
        text = str(slot.get("missing_text", u"（计算结果待补充）"))

    def _expand(m):
        # type: (Any) -> str
        v, _o = ctx.resolve(m.group(1), case_i)
        return str(v) if v is not None else m.group(0)
    text = re.sub(r"\{([A-Za-z0-9_.]+)\}", _expand, text)
    for pa, pb in iter_paragraph_spans(block_xml):
        para = block_xml[pa:pb]
        if tdroox.fragment_text(para).strip().startswith(phrase):
            found = [False]

            def _sub(m):
                # type: (Any) -> str
                if found[0]:
                    return m.group(1) + m.group(2)
                found[0] = True
                return m.group(1) + tdroox.xml_escape(text) + m.group(2)
            new_para = re.sub(r"(<w:t[^>]*>)[^<]*(</w:t>)", _sub, para)
            return replace_sub(block_xml, pa, pb, new_para), True
    ctx.warn(u"条件措辞槽锚点 %r 未命中段落" % phrase)
    return block_xml, False


def _eval_when(expr, verdict):
    # type: (str, Any) -> bool
    m = re.match(r"^\s*[A-Za-z0-9_.]+\s*(>=|<=|>|<|==)\s*(-?[\d.]+)\s*$", expr)
    if not m:
        return False
    op, num = m.group(1), float(m.group(2))
    try:
        v = float(verdict)
    except (TypeError, ValueError):
        return False
    # BUG-04：比较表键与正则捕获 op 严格一致（旧键 " >" 带前导空格永不命中，
    # 严格 ">" 静默落 missing_text）；>= 与 > 语义不得混用
    return {">": v > num, ">=": v >= num, "<": v < num,
            "<=": v <= num, "==": v == num}.get(op, False)


# ---------------------------------------------------------------------------
# M4 循环章实例化（C3-02 终审语义）
# ---------------------------------------------------------------------------

def instantiate_loop(blocks, loop_rule, slot_targets, ctx, registry, box):
    # type: (List[Dict[str, Any]], Dict[str, Any], Dict[int, List[str]], Ctx, Any, Tuple[int, int]) -> Tuple[List[Dict[str, Any]], List[int]]
    """按 02 §2.2 L3 重建模实例化（U-05 每工况完整章）。返回 (新块列表,
    loop 内槽序号列表)。①模板块=template_range 剔除 exclude_subsections；
    ②dead_chapters 整章废弃；③每工况深拷贝模板块 N 份；④literal_rewrite
    （跨 run 匹配+域回避 R-d）+题注隐式承接（R3-01）；⑤renumber K_i=5+i
    （renumber 先、literal 后——C3-02 执行序；子节连续重排 K_i.1/.2/.3；
    结论章=N+6 following_shift 仅 N>=1）；⑥书签唯一化+REF 指令同步改写+
    _GoBack 剥离（H-11）。"""
    from_h = loop_rule["template_range"]["from_heading"]
    to_h = loop_rule["template_range"]["to_before_heading"]
    start = end = None
    for i, b in enumerate(blocks):
        meta = b.get("meta") or {}
        if start is None and meta.get("is_heading") and meta.get("heading") == from_h:
            start = i
        elif start is not None and meta.get("is_heading") and meta.get("heading") == to_h:
            end = i
            break
    if start is None or end is None:
        raise EngineError(u"循环章 template_range 解析失败（%r→%r）" % (from_h, to_h))
    # ① 死子节剔除区间（子节标题块→下一标题块前）
    excluded = set()
    for name in loop_rule["template_range"].get("exclude_subsections", []) or []:
        ei = None
        for i in range(start, end):
            meta = blocks[i].get("meta") or {}
            if (meta.get("is_subsection") or meta.get("is_heading")) \
                    and meta.get("heading") == name:
                ei = i
                break
        if ei is None:
            ctx.warn(u"exclude_subsections %r 未命中子节标题" % name)
            continue
        excluded.add(ei)
        j = ei + 1
        while j < end:
            meta2 = blocks[j].get("meta") or {}
            if meta2.get("is_subsection") or meta2.get("is_heading"):
                break
            excluded.add(j)
            j += 1
    template_idx = [i for i in range(start, end) if i not in excluded]
    range_ids = set(blocks[i]["id"] for i in template_idx)
    # 槽分区：目标块全部落在模板范围=loop 内槽；其余=全局槽
    internal_slots = []
    global_slots = []
    for si, ids in slot_targets.items():
        if ids and all(x in range_ids for x in ids):
            internal_slots.append(si)
        else:
            global_slots.append(si)
    # 子节序号表（连续重排：模板子节出现序 1..o——6.1/6.2/6.4→.1/.2/.3）
    sub_ordinal = {}
    o = 0
    for i in template_idx:
        if (blocks[i].get("meta") or {}).get("is_subsection"):
            o += 1
            sub_ordinal[i] = o
    cases = ctx.dataset.get("cases") or []
    n = len(cases)
    max_bm = _max_bookmark_id("".join(b["xml"] for b in blocks))
    # CA-12：模板范围内书签名集合——REF/PAGEREF 指令只对目标在范围内的改写
    range_bm_names = set()
    for i in template_idx:
        for bm in re.findall(r'<w:bookmarkStart w:id="\d+" w:name="([^"]+)"',
                             blocks[i]["xml"]):
            range_bm_names.add(bm)
    literal = ""
    for lr in loop_rule.get("literal_rewrite", []) or []:
        literal = str(lr.get("match", ""))
        break
    copies = []
    slot_applied = {}     # type: Dict[int, int]    # CC-11：loop 内槽命中计数（按实例累计）
    slot_value_seen = {}  # type: Dict[int, bool]  # CC-11：实例键是否有值（区分失配与缺项）
    # BUG-07：loop 内 text 槽按锚点短语分组（声明序），每实例副本内经计划-应用
    # 两阶段分区（共享锚点整数/all 混合声明互不错位）；非 text 槽维持逐槽应用
    internal_text_groups = []   # type: List[Tuple[str, List[int], List[int]]]  # (phrase, [si], host_ps)
    group_of_phrase = {}        # type: Dict[str, int]
    internal_non_text = []      # type: List[int]
    for si in internal_slots:
        slot = ctx.rules["slots"][si]
        if slot.get("kind") == "text":
            phrase = str((slot.get("anchor") or {}).get("match", ""))
            if phrase in group_of_phrase:
                internal_text_groups[group_of_phrase[phrase]][1].append(si)
            else:
                ids = slot_targets.get(si, [])
                ps = sorted(p for p, i in enumerate(template_idx)
                            if blocks[i]["id"] in ids)
                group_of_phrase[phrase] = len(internal_text_groups)
                internal_text_groups.append((phrase, [si], ps))
        else:
            internal_non_text.append(si)
    for ci, case in enumerate(cases, 1):
        k_i = 5 + ci   # ⑤ H-06 显式公式：K_i=5+i → 章号 6..N+5
        case_name = str(case.get("name") or u"")
        inst = []
        for p, i in enumerate(template_idx):
            xml = blocks[i]["xml"]
            meta = blocks[i].get("meta") or {}
            xml = _strip_cursor_bookmark(xml)          # ⑥ H-11
            if ci >= 2:
                xml, max_bm = _uniquify_bookmarks(xml, ci, max_bm)
                xml = _rewire_ref_instructions(xml, ci, range_bm_names, ctx,
                                               "%s_i%d" % (blocks[i]["id"], ci))
            if meta.get("is_heading"):
                xml = _renumber_leading_number(xml, str(k_i), number_pat=r"^(\d+)(\s)")
            if i in sub_ordinal:
                xml = _renumber_leading_number(
                    xml, "%d.%d" % (k_i, sub_ordinal[i]), number_pat=r"^(\d+\.\d+)")
            # ④ literal_rewrite（renumber 先、literal 后，作用于同段不同 run）
            xml = _literal_rewrite(xml, literal, case_name)
            # ④' 题注隐式承接：SEQ 题注段域后子跨度工况名改写（R3-01）
            xml = _caption_implicit(xml, literal, case_name)
            inst.append({"id": "%s_i%d" % (blocks[i]["id"], ci), "xml": xml,
                         "meta": meta})
        # 槽绑定（loop 内槽逐实例；slot 序号序=句内槽先于整句槽 C3-07）
        for phrase, sis, host_ps in internal_text_groups:
            for si in sis:
                if ctx.resolve(str(ctx.rules["slots"][si].get("bind", "")), ci)[0] is not None:
                    slot_value_seen[si] = True
            if not host_ps:
                continue
            xmls = [inst[p]["xml"] for p in host_ps]
            entries = [(si, ctx.rules["slots"][si]) for si in sis]
            new_xmls, applied, _total = _plan_apply_text_group(xmls, entries, phrase,
                                                               ctx, ci)
            for j, p in enumerate(host_ps):
                if new_xmls[j] != inst[p]["xml"]:
                    inst[p]["xml"] = new_xmls[j]
            for si in sis:
                slot_applied[si] = slot_applied.get(si, 0) + applied.get(si, 0)
        for si in internal_non_text:
            ids = slot_targets.get(si, [])
            slot = ctx.rules["slots"][si]
            for p, i in enumerate(template_idx):
                if blocks[i]["id"] in ids:
                    inst[p]["xml"], used = _apply_slot(inst[p]["xml"], slot, ctx, ci,
                                                       registry, box)
                    slot_applied[si] = slot_applied.get(si, 0) + (1 if used else 0)
                    break
        copies.append(inst)
    # CC-11：loop 内槽全实例零命中=降级告警（C3-07；仅在有值可写时——
    # 无值路径已由槽级缺项告警承担）
    for si in internal_slots:
        if slot_value_seen.get(si) and slot_applied.get(si, 0) == 0:
            slot = ctx.rules["slots"][si]
            ctx.warn(u"循环内槽 %r 全部实例未命中任何可替换跨度（原样保留，C3-07）"
                     % str((slot.get("anchor") or {}).get("match")
                           or (slot.get("anchor") or {}).get("match_prefix", "")))
    # ② dead_chapters 废弃（整章移除）
    dead = set()
    for name in loop_rule.get("dead_chapters", []) or []:
        for i, b in enumerate(blocks):
            meta = b.get("meta") or {}
            if meta.get("is_heading") and meta.get("heading") == name:
                dead.add(i)
                j = i + 1
                while j < len(blocks):
                    meta2 = blocks[j].get("meta") or {}
                    if meta2.get("is_heading"):
                        break
                    dead.add(j)
                    j += 1
    if dead:
        ctx.warn(u"死章废弃移除块：%s" % ", ".join(blocks[i]["id"] for i in sorted(dead)))
    prefix = [blocks[i] for i in range(0, start) if i not in dead]
    suffix = [blocks[i] for i in range(end, len(blocks)) if i not in dead]
    # ⑤ following_shift：loop 之后章标题字面重排（结论章=N+6；N=0 跳过——
    # 空数据导出基准②差异最小化）
    if n >= 1:
        suffix = _following_shift(suffix, n)
    new_blocks = prefix + [b for inst in copies for b in inst] + suffix
    return new_blocks, internal_slots


def _max_bookmark_id(xml):
    # type: (str) -> int
    ids = [int(x) for x in re.findall(r'<w:bookmark(?:Start|End) w:id="(\d+)"', xml)]
    return max(ids) if ids else 0


def _strip_cursor_bookmark(xml):
    # type: (str) -> str
    for m in re.finditer(r'<w:bookmarkStart w:id="(\d+)" w:name="_GoBack"\s*/>', xml):
        xml = tdroox.strip_bookmark_by_id(xml, m.group(1))
    return xml


def _uniquify_bookmarks(xml, ci, max_bm):
    # type: (str, int, int) -> Tuple[str, int]
    """⑥ 书签唯一化：w:name 加 _i 后缀；w:id 全局重编（自 max_bm+1 顺序分配，
    Start/End 按 id 配对同步改写）。"""
    id_map = {}   # type: Dict[str, str]
    mx = [max_bm]

    def _start(m):
        # type: (Any) -> str
        old = m.group(1)
        if old not in id_map:
            mx[0] += 1
            id_map[old] = str(mx[0])
        return '<w:bookmarkStart w:id="%s" w:name="%s_%d"' % (id_map[old], m.group(2), ci)

    xml = re.sub(r'<w:bookmarkStart w:id="(\d+)" w:name="([^"]+)"', _start, xml)

    def _end(m):
        # type: (Any) -> str
        return '<w:bookmarkEnd w:id="%s"' % id_map.get(m.group(1), m.group(1))

    xml = re.sub(r'<w:bookmarkEnd w:id="(\d+)"', _end, xml)
    return xml, mx[0]


def _rewire_ref_instructions(xml, ci, range_bm_names=None, ctx=None, blk_id=""):
    # type: (str, int, Optional[set], Optional[Ctx], str) -> str
    """⑥ REF 指令同步改写：REF <name> → REF <name>_<ci>（书签名后缀联动）。
    CA-12：①只改写目标书签落在循环模板块集内的指令（目标在范围外→保留原名
    +告警，防实例副本内悬空）；②正则加 (?<![A-Za-z_]) 词边界——PAGEREF 的
    "REF" 子串不再被连带命中。range_bm_names 缺省 None=不过滤（兼容旧直调
    形态，引擎主链恒传模板块书签集）。"""
    out = []
    pos = 0
    warned = set()   # type: set
    for m in re.finditer(r"(?<![A-Za-z_])((?:PAGEREF|REF)\s+)([A-Za-z_][A-Za-z0-9_]*)(\s)",
                         xml):
        name = m.group(2)
        if range_bm_names is not None and name not in range_bm_names:
            if ctx is not None and name not in warned:
                warned.add(name)
                ctx.warn(u"实例 %d 块 %s 的域指令目标书签 %r 不在循环模板块内，"
                         u"保留原名不改写（CA-12 防悬空）" % (ci, blk_id, name))
            continue
        if not name.endswith("_%d" % ci):
            out.append(xml[pos:m.start()])
            out.append("%s%s_%d%s" % (m.group(1), name, ci, m.group(3)))
            pos = m.end()
    out.append(xml[pos:])
    return "".join(out)


def _renumber_leading_number(xml, new_number, number_pat):
    # type: (str, str, str) -> str
    """前导编号改写（'6'/'6.1' 实测单 run，评审 3.2 终审确认机制可行）：
    段拼接文本匹配 number_pat→首个 w:t 承载新编号+其内余文，其余命中字符
    清空（跨 run 防御）。"""
    for pa, pb in iter_paragraph_spans(xml):
        para = xml[pa:pb]
        text = tdroox.fragment_text(para)
        m = re.match(number_pat, text)
        if not m:
            continue
        old_prefix = m.group(1)
        # 逐 w:t 消费旧编号字符：首个 w:t 承载新编号+其内余文，其余命中字符清空
        # （'6'/'6.1'/'12' 实测单 run——跨 run 为防御路径）
        take = [len(old_prefix)]
        first = [True]

        def _sub(mm):
            # type: (Any) -> str
            if take[0] <= 0:
                return mm.group(0)
            t = mm.group(2)
            erase = min(take[0], len(t))
            take[0] -= erase
            newt = (new_number if first[0] else "") + t[erase:]
            first[0] = False
            return mm.group(1) + tdroox.xml_escape(newt) + mm.group(3)
        para2 = re.sub(r"(<w:t[^>]*>)([^<]*)(</w:t>)", _sub, para)
        if take[0] > 0:
            continue   # 编号未完整落写（跨 run 异常形态，防御跳过）
        return replace_sub(xml, pa, pb, para2)
    return xml


def _literal_rewrite(xml, literal, case_name):
    # type: (str, str, str) -> str
    """④ 非高亮字面改写（R-d 域回避）：跨 run 拼接匹配（w:br/w:tab 与域体
    为边界）仅命中非高亮文本 run；命中 run 序列文本改写、run 结构与 rPr 保持。"""
    if not literal or not case_name or literal == case_name:
        return xml
    for _attempt in range(64):
        target = None
        for pa, pb in iter_paragraph_spans(xml):
            para = xml[pa:pb]
            if literal not in tdroox.fragment_text(para):
                continue
            runs = tdroox.iter_runs(para)
            hits = tdroox.match_literal_in_runs(runs, literal)
            if hits:
                run_spans = [(r.start, r.end) for r in runs]
                new_para = tdroox.rewrite_runs_with_text(para, run_spans, hits[0], case_name)
                target = (pa, pb, new_para)
                break
        if target is None:
            return xml
        xml = replace_sub(xml, target[0], target[1], target[2])
    return xml


def _caption_implicit(xml, literal, case_name):
    # type: (str, str, str) -> str
    """④' 题注隐式承接（R3-01/C3-01 R-b 同源）：含 SEQ 域且带高亮子跨度的
    题注段——域后子跨度文本中的字面工况名改写为实例工况名（fldChar/instrText
    结构保留；整段清高亮由收口统一完成）。"""
    if not literal or literal == case_name:
        return xml
    for pa, pb in iter_paragraph_spans(xml):
        para = xml[pa:pb]
        if "SEQ" not in para or literal not in tdroox.fragment_text(para):
            continue
        runs = tdroox.iter_runs(para)
        changed = False
        for sp in tdroox.merge_highlight_spans(runs):
            if sp["in_field"] or sp["in_field_result"] or literal not in sp["text"]:
                continue
            # 域外子跨度内字面→case_name：跨度内字符级改写（run 结构保持）
            run_lo, run_hi = sp["run_range"]
            seg_runs = runs[run_lo:run_hi + 1]
            seg_text = "".join(r.text for r in seg_runs)
            at = seg_text.find(literal)
            if at < 0:
                continue
            new_seg = seg_text[:at] + case_name + seg_text[at + len(literal):]
            run_spans_all = [(r.start, r.end) for r in runs]
            hit = (run_lo, run_hi, 0, len(seg_text))
            para = tdroox.rewrite_runs_with_text(para, run_spans_all, hit, new_seg)
            changed = True
            break
        if changed:
            xml = replace_sub(xml, pa, pb, para)
    return xml


def _apply_slot(xml, slot, ctx, case_i, registry, box, occ_offset=0):
    # type: (str, Dict[str, Any], Ctx, Optional[int], Any, Tuple[int, int], int) -> Tuple[str, int]
    """应用单槽，返回 (新 XML, 命中消耗数)——消耗数供 occurrence 全文档序
    语义（CC-03）与失配告警（CC-11）使用；非文本槽=1/0 表示是否生效。"""
    kind = slot.get("kind")
    if kind == "text":
        return replace_text_slot(xml, slot, ctx, case_i, occ_offset)
    if kind == "table":
        out, ok = process_table_slot(xml, slot, ctx, case_i)
        return out, (1 if ok else 0)
    if kind == "image":
        out, ok = process_image_slot(xml, slot, ctx, case_i, registry, box)
        return out, (1 if ok else 0)
    if kind == "conditional_text":
        out, ok = render_conditional(xml, slot, ctx, case_i)
        return out, (1 if ok else 0)
    return xml, 0   # ole=preserve（C8）：资产原样搬运，零动作


def _following_shift(blocks, n):
    # type: (List[Dict[str, Any]], int) -> List[Dict[str, Any]]
    """loop 之后章标题字面重排：首个后续章=N+6（K 末=5+n），逐章接续。
    仅作用 is_heading 章级块（前导数字实测单 run）。"""
    next_no = 5 + n + 1
    out = list(blocks)
    for k, b in enumerate(out):
        meta = b.get("meta") or {}
        if not meta.get("is_heading"):
            continue
        out[k] = {"id": b["id"], "xml": _renumber_leading_number(
            b["xml"], str(next_no), number_pat=r"^(\d+)(\s)"), "meta": meta}
        next_no += 1
    return out


# ---------------------------------------------------------------------------
# 槽→宿主块定位（B4：容器模板块唯一命中语义）
# ---------------------------------------------------------------------------

def build_slot_targets(rules, blocks):
    # type: (Dict[str, Any], List[Dict[str, Any]]) -> Dict[int, List[str]]
    """槽锚点→宿主块 id 表：文本/图片形态1=字面命中块；表格槽=题注字面块→
    其后首个表块（或表内文本直接命中）；图片表格坐标=table_match 命中表块；
    图片空段坐标=anchor.block；条件措辞=前缀命中块；ole=ProgID 所在块。"""
    targets = {}   # type: Dict[int, List[str]]
    for si, slot in enumerate(rules.get("slots", [])):
        kind = slot.get("kind")
        anchor = slot.get("anchor") or {}
        ids = []   # type: List[str]
        if kind in ("text",) or (kind == "image" and anchor.get("match")):
            phrase = str(anchor.get("match", ""))
            ids = [b["id"] for b in blocks if phrase and phrase in tdroox.fragment_text(b["xml"])]
        elif kind == "table":
            phrase = str(anchor.get("match", ""))
            cap_at = None
            for i, b in enumerate(blocks):
                if phrase and phrase in tdroox.fragment_text(b["xml"]):
                    cap_at = i
                    if (b.get("meta") or {}).get("kind") == "table":
                        ids = [b["id"]]
                        break
            if cap_at is not None and not ids:
                for j in range(cap_at + 1, len(blocks)):
                    if (blocks[j].get("meta") or {}).get("kind") == "table":
                        ids = [blocks[j]["id"]]
                        break
        elif kind == "image" and anchor.get("element") == "table":
            phrase = str(anchor.get("table_match", ""))
            for b in blocks:
                if (b.get("meta") or {}).get("kind") == "table" \
                        and phrase in tdroox.fragment_text(b["xml"]):
                    ids.append(b["id"])
        elif kind == "image" and anchor.get("element") == "block_paragraph":
            ids = [str(anchor.get("block", ""))]
        elif kind == "conditional_text":
            phrase = str(anchor.get("match_prefix", ""))
            ids = [b["id"] for b in blocks
                   if phrase and tdroox.fragment_text(b["xml"]).strip().startswith(phrase)]
        elif kind == "ole":
            pid = str(anchor.get("ole_prog_id", ""))
            ids = [b["id"] for b in blocks if pid and pid in b["xml"]]
        targets[si] = ids
    return targets


# ---------------------------------------------------------------------------
# M4 主装配
# ---------------------------------------------------------------------------

def assemble(con, dataset):
    # type: (Container, Dict[str, Any]) -> Tuple[List[Dict[str, Any]], Ctx, Any]
    """M4：循环章实例化（含 loop 内槽绑定）→全局槽→全篇清高亮收口。"""
    ctx = Ctx(dataset, con.rules)
    source_names = [con.store_to_source.get(s, s) for s in con.store_bytes.keys()]
    doc_rels = (con.store_bytes.get("assets/rels/document.xml.rels") or b"").decode("utf-8")
    if "</Relationships>" not in doc_rels:
        # CC-02：精简包可无 document.xml.rels——以合法空根初始化（登记媒体/
        # 图片关系时照常续号追加）
        doc_rels = ('<Relationships xmlns="http://schemas.openxmlformats.org'
                    '/package/2006/relationships"></Relationships>')
    registry = tdroox.MediaRegistry(
        source_names,
        doc_rels,
        con.manifest.get("content_types", ""),
        media_dir="word/media/")
    sections = con.index.get("sections") or []
    sect = sections[-1] if sections else {}
    margins = sect.get("margins") or [1418, 1055, 2608, 2325]
    sectpr = ('<w:pgSz w:w="%d" w:h="%d"/>'
              '<w:pgMar w:top="%d" w:right="%d" w:bottom="%d" w:left="%d"/>') % (
        sect.get("sect_width", 11906), sect.get("sect_height", 16838),
        margins[0] if len(margins) > 0 else 1418,
        margins[1] if len(margins) > 1 else 1055,
        margins[2] if len(margins) > 2 else 2608,
        margins[3] if len(margins) > 3 else 2325)
    box = (tdroox.body_width_emu(sectpr), tdroox.body_width_emu(sectpr) * 3)
    blocks = list(con.blocks)
    slot_targets = build_slot_targets(con.rules, blocks)
    all_internal = []
    for lp in con.rules.get("loop_chapters", []) or []:
        blocks, internal = instantiate_loop(blocks, lp, slot_targets, ctx,
                                            registry, box)
        all_internal.extend(internal)
    # 全局槽（目标块在 loop 外）：句内槽先于整句槽=slot 声明序（C3-07）。
    # BUG-07：text 槽按锚点短语分组（声明序），组内共享锚点经计划-应用两阶段
    # 在原文命中快照上一次性分区（整数=第 k 命中、all=剩余全部、越界=显式
    # 告警不静默）；非 text 槽维持逐槽应用（自带失配/缺项告警）。
    # 应用严格按槽声明序推进（text 组在其首成员处触发一次）——XXX 句内槽先于
    # 整句条件措辞槽等跨 kind 相对序不得改变
    text_groups = []          # type: List[List[int]]  # 组内槽 si（声明序）
    group_of_phrase = {}      # type: Dict[str, int]
    for si, slot in enumerate(con.rules.get("slots", [])):
        if si in all_internal:
            continue
        if slot.get("kind") != "text":
            continue
        phrase = str((slot.get("anchor") or {}).get("match", ""))
        if phrase in group_of_phrase:
            text_groups[group_of_phrase[phrase]].append(si)
        else:
            group_of_phrase[phrase] = len(text_groups)
            text_groups.append([si])
    applied_groups = set()    # type: set
    for si, slot in enumerate(con.rules.get("slots", [])):
        if si in all_internal:
            continue
        kind = slot.get("kind")
        if kind == "ole":
            continue   # preserve：零动作
        if kind == "text":
            phrase = str((slot.get("anchor") or {}).get("match", ""))
            gi = group_of_phrase[phrase]
            if gi in applied_groups:
                continue   # 同组后续槽已随首成员一次分区应用
            applied_groups.add(gi)
            host_ids = set()
            for sj in text_groups[gi]:
                host_ids.update(slot_targets.get(sj, []))
            host_ks = [k for k, b in enumerate(blocks) if b["id"] in host_ids]
            if not host_ks:
                continue
            xmls = [blocks[k]["xml"] for k in host_ks]
            entries = [(sj, con.rules["slots"][sj]) for sj in text_groups[gi]]
            new_xmls, _applied, _total = _plan_apply_text_group(xmls, entries, phrase,
                                                                ctx, None)
            for j, k in enumerate(host_ks):
                if new_xmls[j] != blocks[k]["xml"]:
                    blocks[k] = {"id": blocks[k]["id"], "xml": new_xmls[j],
                                 "meta": blocks[k].get("meta")}
            # CC-11/C3-07 失配告警已由 _plan_apply_text_group 内部按槽显式发出
            # （越界/冲突/零命中，仅在有值可写时——无值路径由槽级缺项告警承担）
        else:
            for k, b in enumerate(blocks):
                if b["id"] not in slot_targets.get(si, []):
                    continue
                xml2, _used = _apply_slot(b["xml"], slot, ctx, None, registry, box)
                if xml2 != b["xml"]:
                    blocks[k] = {"id": blocks[k]["id"], "xml": xml2,
                                 "meta": blocks[k].get("meta")}
    # 全篇清高亮收口（highlight_clear：run rPr 与段落标记 rPr 双处，T-22）
    for k, b in enumerate(blocks):
        xml2 = tdroox.clear_highlight_xml(b["xml"])
        if xml2 != b["xml"]:
            blocks[k] = {"id": b["id"], "xml": xml2, "meta": b.get("meta")}
    return blocks, ctx, registry


# ---------------------------------------------------------------------------
# M5 打包与域处理
# ---------------------------------------------------------------------------

def rewrite_seq_caches(body):
    # type: (str) -> Tuple[str, Dict[str, int]]
    """SEQ 域结果缓存按文档序重算（B7/M5：F9 之前缓存无重复编号且与实例序
    一致）。TOC 缓存 PAGEREF 豁免（C3-08）。"""
    token = re.compile(r'<w:fldChar w:fldCharType="(begin|separate|end)"[^>]*/>'
                       r"|<w:instrText[^>]*>([^<]*)</w:instrText>")
    pieces = []   # type: List[Tuple[int, int, str]]
    depth = []    # type: List[Dict[str, Any]]
    counters = {}  # type: Dict[str, int]
    for m in token.finditer(body):
        if m.group(1):
            kind = m.group(1)
            if kind == "begin":
                depth.append({"instr": "", "sep": None})
            elif kind == "separate":
                if depth:
                    depth[-1]["sep"] = m.end()
            elif kind == "end":
                if depth:
                    f = depth.pop()
                    if f["sep"] is not None and f["instr"].strip().startswith("SEQ"):
                        words = f["instr"].strip().split()
                        name = words[1] if len(words) > 1 else "SEQ"
                        counters[name] = counters.get(name, 0) + 1
                        pieces.append((f["sep"], m.start(), str(counters[name])))
        else:
            if depth:
                depth[-1]["instr"] += (m.group(2) or "")
    # 单趟自右向左拼装（CC-10：逐片 `out[:start]+…+out[end:]` 为 O(片数×全长)
    # 字节复制热点；本文本切片均为域内小块，O(N) 拼装语义不变）
    out = body
    last = len(body)
    buf = []
    for start, end, new in sorted(pieces, key=lambda x: -x[0]):
        seg = out[start:end]
        found = [False]

        def _sub(mm):
            # type: (Any) -> str
            if found[0]:
                return mm.group(1) + mm.group(2)
            found[0] = True
            return mm.group(1) + tdroox.xml_escape(new) + mm.group(2)
        buf.append(out[end:last])
        buf.append(re.sub(r"(<w:t[^>]*>)[^<]*(</w:t>)", _sub, seg))
        last = start
    buf.append(out[:last])
    return "".join(reversed(buf)), counters


def rewrite_ref_caches(body):
    # type: (str) -> str
    """REF 域缓存尾数联动（B7）：REF 目标书签所在题注段的 SEQ 缓存新值→
    REF 缓存尾随数字替换（"表7"→"表4"）；无数字形态原样保留（F9 自愈）。
    CC-10：SEQ 缓存定位走排序位置表+bisect、段界判定走段落终点表+bisect、
    SEQ 缓存探测窗口限幅——消除 O(REF×SEQ) 线性扫描与全文切片热点；
    替换单趟自右向左拼装。语义与原实现逐字一致。"""
    bm_pos = {}   # type: Dict[str, int]
    for m in re.finditer(r'<w:bookmarkStart w:id="\d+" w:name="([^"]+)"', body):
        bm_pos.setdefault(m.group(1), m.start())
    # SEQ 缓存数字位置表（rewrite_seq_caches 已先行执行——值为新值；探测窗口
    # 限幅 3000 字符，避免逐 instrText 切片全文）
    seq_at = []   # type: List[Tuple[int, str]]
    for m in re.finditer(r'<w:instrText[^>]*>([^<]*)</w:instrText>', body):
        instr = (m.group(1) or "").strip()
        if not instr.startswith("SEQ"):
            continue
        seg = body[m.end():m.end() + 3000]
        em = re.search(r'<w:fldChar w:fldCharType="separate"[^>]*/>', seg)
        if not em:
            continue
        cache = body[m.end() + em.end():m.end() + em.end() + 3000]
        tm = re.search(r"(<w:t[^>]*>)([^<]*)</w:t>", cache)
        if tm:
            seq_at.append((m.end() + em.end() + tm.start(2),
                           instr.split()[1] if len(instr.split()) > 1 else ""))
    seq_positions = sorted(sp for sp, _name in seq_at)
    para_ends = sorted(m.end() for m in re.finditer(r"</w:p>", body))
    pieces = []   # type: List[Tuple[int, int, str]]
    token = re.compile(r'<w:fldChar w:fldCharType="(begin|separate|end)"[^>]*/>'
                       r"|<w:instrText[^>]*>([^<]*)</w:instrText>")
    depth = []   # type: List[Dict[str, Any]]
    for m in token.finditer(body):
        if m.group(1) == "begin":
            depth.append({"instr": "", "sep": None})
        elif m.group(1) == "separate":
            if depth:
                depth[-1]["sep"] = m.end()
        elif m.group(1) == "end":
            if depth:
                f = depth.pop()
                if f["sep"] is None or not f["instr"].strip().startswith("REF"):
                    continue
                words = f["instr"].strip().split()
                target = words[1] if len(words) > 1 else ""
                pos = bm_pos.get(target)
                if pos is None:
                    continue
                # 书签之后首个 SEQ 缓存（原线性扫描→bisect；语义=同段首个）
                si = bisect.bisect_right(seq_positions, pos)
                sv = None
                if si < len(seq_positions):
                    sp = seq_positions[si]
                    # pos..sp 之间出现段界（</w:p>）→目标段无同段 SEQ
                    pi = bisect.bisect_right(para_ends, pos)
                    if pi < len(para_ends) and para_ends[pi] < sp:
                        sv = None
                    else:
                        sv = sp
                if sv is None:
                    continue
                seg = body[f["sep"]:m.start()]
                tm = None
                for tmm in re.finditer(r"(<w:t[^>]*>)([^<]*)(</w:t>)", seg):
                    tm = tmm
                if tm is None or not re.search(r"\d+$", tm.group(2)):
                    continue
                new_tail = re.sub(r"\d+$", lambda _m: body[sv:sv + 8].split("<")[0], tm.group(2))
                s = f["sep"] + tm.start(2)
                pieces.append((s, s + len(tm.group(2)), new_tail))
        else:
            if depth:
                depth[-1]["instr"] += (m.group(2) or "")
    out = body
    last = len(body)
    buf = []
    for start, end, new in sorted(pieces, key=lambda x: -x[0]):
        buf.append(out[end:last])
        buf.append(tdroox.xml_escape(new))
        last = start
    buf.append(out[:last])
    buf.reverse()
    return "".join(buf)


def package_doc(con, blocks, registry, dataset):
    # type: (Container, List[Dict[str, Any]], Any, Dict[str, Any]) -> Dict[str, bytes]
    """M5：资产化部件逐字节落包+document.xml 重组+docPr 重算+SEQ/REF 缓存
    改写+TOC 置 dirty（空数据豁免）+媒体接力+收尾 rels Target 复核。"""
    parts = {}   # type: Dict[str, bytes]
    for store, data in con.store_bytes.items():
        src = con.store_to_source.get(store)
        if not src or store.startswith("assets/content/"):
            continue   # content（块/index）经 document.xml 重组落包
        parts[src] = data
    body = "".join(b["xml"] for b in blocks)
    doc = (con.manifest.get("document_xml_prefix", "") + "<w:body>" + body
           + "</w:body>" + con.manifest.get("document_xml_suffix", "</w:document>"))
    seq = [0]

    def _renum(m):
        # type: (Any) -> str
        seq[0] += 1
        return '<wp:docPr id="%d"' % seq[0]
    doc = re.sub(r'<wp:docPr id="\d+"', _renum, doc)
    doc, _counters = rewrite_seq_caches(doc)
    doc = rewrite_ref_caches(doc)
    parts["word/document.xml"] = doc.encode("utf-8")
    parts["word/_rels/document.xml.rels"] = registry.rels_xml.encode("utf-8")
    parts["[Content_Types].xml"] = registry.content_types.encode("utf-8")
    parts["_rels/.rels"] = con.manifest.get("package_rels", "").encode("utf-8")
    # TOC 置 dirty（R4 双态口径：空数据导出不注入，settings.xml 保持逐字节一致）
    if len(dataset.get("cases") or []) > 0 and "word/settings.xml" in parts:
        settings = parts["word/settings.xml"].decode("utf-8")
        if "<w:updateFields" not in settings:
            m = re.search(r"<w:settings\b[^>]*>", settings)
            if m:
                settings = settings[:m.end()] + '<w:updateFields w:val="true"/>' + settings[m.end():]
                parts["word/settings.xml"] = settings.encode("utf-8")
    parts.update(registry.drain_media())
    # 收尾：全 rels Target 在包内存在复核（T-01-③，悬空=阻断）
    missing = []
    xml = parts.get("word/_rels/document.xml.rels", b"").decode("utf-8")
    for m in re.finditer(r'<Relationship\b([^>]*)/>', xml):
        attrs = m.group(1)
        if 'TargetMode="External"' in attrs:
            continue
        tm = re.search(r'\bTarget="([^"]+)"', attrs)
        if not tm:
            continue
        t = tm.group(1).replace("\\", "/").lstrip("/")
        full = posixpath.normpath(t if t.startswith("word/") else "word/" + t)
        if full not in parts:
            missing.append(t)
    if missing:
        raise EngineError(u"输出包 rels 悬空（阻断，T-01-③）：%s" % "; ".join(missing[:5]))
    # 输出侧 sanity 收口（CC-04/CC-05/CC-09 共用兜底面，E5 结构性损坏阻断）：
    # document.xml 落包前 expat 解析自检（任何手术引入的畸形=拒载零产出，
    # rc 如实反映失败）；[Content_Types].xml 必须非空且可解析。
    for part_name, data in (("word/document.xml", parts.get("word/document.xml", b"")),
                            ("[Content_Types].xml", parts.get("[Content_Types].xml", b""))):
        if not data.strip():
            raise EngineError(u"输出包关键部件 %s 为空（结构性损坏，阻断）" % part_name)
        parser = expat.ParserCreate()
        try:
            parser.Parse(data, True)
        except expat.ExpatError as e:
            raise EngineError(u"输出包 %s 结构性损坏（well-formed 自检失败，阻断）：%s"
                              % (part_name, e))
    return parts


# ---------------------------------------------------------------------------
# finish_atomic（T-13/A-01 无 pre-delete；C3-09 文案映射）
# ---------------------------------------------------------------------------

def finish_atomic(tmp_path, out_path):
    # type: (str, str) -> None
    try:
        os.replace(tmp_path, out_path)
    except PermissionError:
        _cleanup(tmp_path)
        # DX-18：与向导定版占用文案同句式骨架（同一用户心智一种说法）
        raise EngineError(u"输出文件正被占用或为只读（可能已在 Word 中打开，"
                          u"或带有只读属性），请解除占用/只读后重试：%s" % out_path)
    except OSError as e:
        _cleanup(tmp_path)
        if e.errno == 28:   # ENOSPC
            raise EngineError(u"磁盘空间不足：%s" % out_path)
        raise EngineError(u"输出目录不可写或路径无效（含 >260 字符长路径）：%s" % out_path)


def _cleanup(tmp_path):
    # type: (str) -> None
    try:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)
    except OSError:
        pass


# ---------------------------------------------------------------------------
# --compare 试导出比对（T-10：基准①②自动 diff+残留计数；③④⑤人工清单）
# ---------------------------------------------------------------------------

def compare_with_source(out_docx, src_docx, report_dir):
    # type: (str, str, str) -> Dict[str, Any]
    report = {"ok": True, "checks": []}   # type: Dict[str, Any]
    src = tdroox.read_package(src_docx)
    out = tdroox.read_package(out_docx)
    byte_diff = []
    for n in sorted(set(src.keys()) & set(out.keys())):
        if n in ("word/document.xml", "word/settings.xml"):
            continue   # document.xml=变换本体；settings=updateFields 注入件
        if src[n] != out[n]:
            byte_diff.append(n)
    report["checks"].append({"name": u"基准①/②资产部件逐字节一致",
                             "ok": not byte_diff,
                             "detail": byte_diff or u"全部一致"})
    if byte_diff:
        report["ok"] = False
    src_body = re.search(r"<w:body>(.*)</w:body>",
                         src["word/document.xml"].decode("utf-8"), re.S).group(1)
    out_body = re.search(r"<w:body>(.*)</w:body>",
                         out["word/document.xml"].decode("utf-8"), re.S).group(1)
    sc = tdroox.top_level_children(src_body)
    oc = tdroox.top_level_children(out_body)
    # 前缀/后缀对齐（索引对位在循环块删除后会整体位移——先对齐公共前缀与
    # 公共后缀，中段=循环区间删除/占位差异区，供人工核对）
    na, nb = len(sc), len(oc)

    def frag(body, ch, i):
        # type: (str, List[Tuple[str, int, int]], int) -> str
        return body[ch[i][1]:ch[i][2]]
    pa = 0
    while pa < min(na, nb) and frag(src_body, sc, pa) == frag(out_body, oc, pa):
        pa += 1
    sb = 0
    while sb < min(na, nb) - pa and frag(src_body, sc, na - 1 - sb) == frag(out_body, oc, nb - 1 - sb):
        sb += 1
    mid_src = [tdroox.fragment_text(frag(src_body, sc, i))[:40]
               for i in range(pa, na - sb)]
    mid_out = [tdroox.fragment_text(frag(out_body, oc, i))[:40]
               for i in range(pa, nb - sb)]
    report["checks"].append({
        "name": u"基准②document.xml 前缀/后缀对齐 diff（差异限占位/循环/插入位置）",
        "src_blocks": na, "out_blocks": nb,
        "prefix_equal_blocks": pa, "suffix_equal_blocks": sb,
        "mid_src_removed_count": len(mid_src), "mid_out_added_count": len(mid_out),
        "mid_src_texts": mid_src[:40], "mid_out_texts": mid_out[:40],
        "manual_review": u"前缀/后缀逐字节相等；中段=循环区间删除/占位替换/插入位置，"
                         u"请人工核对 mid_src/mid_out 文本清单",
    })
    hl_out = len(re.findall(r'w:highlight[^>]*w:val="yellow"',
                            out["word/document.xml"].decode("utf-8")))
    report["checks"].append({"name": u"基准③高亮残留=0（run+段落标记双处）",
                             "ok": hl_out == 0, "count": hl_out})
    if hl_out:
        report["ok"] = False
    fld_out = len(re.findall(r'<w:fldChar w:fldCharType="begin"',
                             out["word/document.xml"].decode("utf-8")))
    fld_src = len(re.findall(r'<w:fldChar w:fldCharType="begin"',
                             src["word/document.xml"].decode("utf-8")))
    instr_out = len(re.findall(r"<w:instrText", out["word/document.xml"].decode("utf-8")))
    report["checks"].append({"name": u"R-e 域结构 fldChar 计数（源/出）",
                             "src_fldchar": fld_src, "out_fldchar": fld_out,
                             "out_instrtext": instr_out,
                             "note": u"实例化后=38+(N-1)×域增量；空数据=28（模板块域移除）"})
    sect_out = re.findall(r"<w:sectPr.*?</w:sectPr>",
                          out["word/document.xml"].decode("utf-8"), re.S)
    hf = [re.findall(r'<w:(?:header|footer)Reference w:type="(\w+)" r:id="(rId\d+)"/>', s)
          for s in sect_out]
    report["checks"].append({"name": u"FR-33 sectPr 数与头脚引用映射",
                             "sectPr": len(sect_out), "refs": hf})
    report["manual_checklist"] = [
        u"④Word 2016+/WPS 双开：打开无修复对话框；F9 全域刷新后 SEQ 连续/REF 正确/TOC 页码正确",
        u"⑤FR-25 断言：每实例章标题字面=K_i+实例工况名；子节=K_i.1/.2/.3 逐值；"
        u"_Ref 书签逐实例唯一化且 REF 指令同步；输出包无 _GoBack 多副本",
    ]
    if not os.path.isdir(report_dir):
        os.makedirs(report_dir)
    with io.open(os.path.join(report_dir, "compare_report.json"), "w",
                 encoding="utf-8") as f:
        f.write(json.dumps(report, ensure_ascii=False, indent=1))
    return report


# ---------------------------------------------------------------------------
# 主流程
# ---------------------------------------------------------------------------

def _reject_nonfinite_constant(name):
    # type: (str) -> float
    """CC-12：json 解析钩子——NaN/Infinity/-Infinity 字面=数据集非法（可读
    文案阻断），不得静默落文档。"""
    raise ValueError(u"数据集含非有限数值常量 %r（NaN/Infinity 不落文档，"
                     u"请改写为 null 或合法数值）" % name)


def _finite_float(text):
    # type: (str) -> float
    """CC-12：1e400 等溢出字面解析为 inf 一并拒绝（与 parse_constant 同口径）。"""
    v = float(text)
    if not math.isfinite(v):
        raise ValueError(u"数据集含非有限数值 %s（NaN/Infinity 不落文档）" % text)
    return v


def _aggregate_warnings(warnings):
    # type: (List[str]) -> List[str]
    """DX-07：同文告警聚合（保持首现顺序，重复项附同款次数）"""
    order = []   # type: List[str]
    counts = {}  # type: Dict[str, int]
    for w in warnings:
        if w in counts:
            counts[w] += 1
        else:
            counts[w] = 1
            order.append(w)
    return [w if counts[w] == 1 else u"%s（同款告警共 %d 次）" % (w, counts[w])
            for w in order]


def _check_dataset_shape(dataset):
    # type: (Dict[str, Any]) -> None
    """数据集形状预检（CC-16：cases/materials/images 元素必须为对象——可读
    文案取代裸 AttributeError；CC-10：cases 数量上限）。
    抛 EngineError（由调用方 emit_failed 结构化输出）。"""
    cases = dataset.get("cases")
    if cases is not None:
        if not isinstance(cases, list):
            raise EngineError(u"数据集 cases 必须为数组（实测 %s）"
                              % type(cases).__name__)
        for i, c in enumerate(cases):
            if not isinstance(c, dict):
                raise EngineError(u"数据集 cases[%d] 必须为对象（实测 %s）"
                                  % (i, type(c).__name__))
        if len(cases) > MAX_CASES:
            raise EngineError(u"数据集工况数 %d 超上限 %d（导出耗时随工况数超线性"
                              u"增长，将超界面超时限制；请拆分项目或分批导出）"
                              % (len(cases), MAX_CASES))
    mats = dataset.get("materials")
    if mats is not None:
        if not isinstance(mats, list):
            raise EngineError(u"数据集 materials 必须为数组")
        for i, m in enumerate(mats):
            if not isinstance(m, dict):
                raise EngineError(u"数据集 materials[%d] 必须为对象" % i)
    imgs = dataset.get("images")
    if imgs is not None:
        if not isinstance(imgs, list):
            raise EngineError(u"数据集 images 必须为数组")
        for i, g in enumerate(imgs):
            if not isinstance(g, dict):
                raise EngineError(u"数据集 images[%d] 必须为对象" % i)


def main(argv):
    # type: (List[str]) -> int
    compare_src = None
    compare_dir = None
    args = []   # type: List[str]
    i = 0
    while i < len(argv):
        if argv[i] == "--compare" and i + 2 <= len(argv) - 1:
            compare_src, compare_dir = argv[i + 1], argv[i + 2]
            i += 3
            continue
        args.append(argv[i])
        i += 1
    if len(args) != 3:
        sys.stderr.write(u"usage: tdr_engine.py <container.rtdoc> <dataset.json> "
                         u"<output.docx> [--compare <src.docx> <report_dir>]\n"
                         u"退出码：0=成功，1=失败（详见输出），2=用法错误\n")
        return 2
    container_path, dataset_path, out_path = args
    tmp_path = out_path + ".tdr_tmp%d" % os.getpid()
    # DX-06：输出路径为目录=显式定位（不再误报"被打开/将被覆盖"）
    if os.path.isdir(out_path):
        _cleanup(tmp_path)
        emit_failed(u"输出路径是目录，请指定 .docx 文件路径：%s" % out_path)
        sys.stderr.write(u"[error] 输出路径是目录，请指定 .docx 文件路径：%s\n" % out_path)
        return 1
    try:
        with io.open(dataset_path, "r", encoding="utf-8") as f:
            dataset = json.load(f, parse_constant=_reject_nonfinite_constant,
                                parse_float=_finite_float)
        if not isinstance(dataset, dict):
            raise EngineError(u"数据集顶层必须是 JSON 对象")
        _check_dataset_shape(dataset)
    except EngineError as e:
        # CC-13：数据集非法也必须发结构化 failed 行（旧实现裸 raise 冒出 main）
        _cleanup(tmp_path)
        emit_failed(e.msg, e.errors)
        sys.stderr.write(u"[error] %s\n" % e.msg)
        return 1
    except ValueError as e:
        emit_failed(u"数据集 JSON 非法：%s" % e)
        sys.stderr.write(u"[error] 数据集 JSON 非法：%s\n" % e)
        return 1
    except OSError as e:
        # DX-02：数据集文件缺失/不可读给可读文案（不再 JSON failed 行后裸 traceback）
        msg = u"数据集文件读取失败：%s（%s；请确认文件存在且可读）" % (dataset_path, e)
        emit_failed(msg)
        sys.stderr.write(u"[error] %s\n" % msg)
        return 1
    except Exception as e:
        emit_failed(u"数据集 JSON 读取失败：%s" % e)
        sys.stderr.write(traceback.format_exc())
        return 1
    try:
        emit_stage("verify", 5, u"容器校验开始")
        con = load_and_verify_container(container_path, dataset)
        emit_stage("load", 15, u"容器已装载（部件 %d，块 %d）"
                   % (len(con.store_bytes), len(con.blocks)))
        emit_stage("locate", 30, u"槽锚点定位完成（唯一命中语义已核）")
        blocks, ctx, registry = assemble(con, dataset)
        emit_stage("transform", 55, u"装配与结构变换完成（块 %d）" % len(blocks))
        parts = package_doc(con, blocks, registry, dataset)
        try:
            with io.open(tmp_path, "wb") as f:
                f.write(b"")   # 输出目录试写预检（C-06/T-19 前置）
        except OSError as e:
            # CA-09：长路径/无权限等试写失败走 C-06 承诺映射（不再落"引擎内部错误"）
            raise EngineError(u"输出目录不可写或路径无效（含 >260 字符长路径）：%s（%s）"
                              % (out_path, e))
        tdroox.write_package(parts, tmp_path)
        # DX-06：pack 阶段改"正在写盘"（旧"打包完成"与随后写盘失败矛盾）
        emit_stage("pack", 85, u"正在写盘（部件 %d）" % len(parts))
        # DX-06："将被覆盖"提示移至写盘前（失败路径不再出现误导性覆盖预告）
        if os.path.exists(out_path):
            sys.stderr.write(u"[warn] 输出文件已存在，将被覆盖：%s\n" % out_path)
        finish_atomic(tmp_path, out_path)
        # 告警汇总（T-22/D-22：按工况聚合 1 条+明细键列表；FR-35 良性告警仍判成功）
        # DX-07：同文告警聚合计数（消除 ×6 重复刷屏）；工况明细按数据集顺序
        warns = _aggregate_warnings(ctx.warnings)
        case_order = {}
        for _ci, _case in enumerate(dataset.get("cases") or []):
            _nm = str(_case.get("name") or "")
            if _nm not in case_order:
                case_order[_nm] = _ci
        for case_name in sorted(ctx.case_warnings.keys(),
                                key=lambda n: (case_order.get(n, 1 << 30), n)):
            warns.append(u"工况 %s 缺项：%s" % (case_name, ", ".join(ctx.case_warnings[case_name])))
        emit_stage("done", 100, u"导出完成：%s%s"
                   % (out_path, ("；告警 %d 条" % len(warns)) if warns else ""))
        for w in warns[:50]:
            sys.stderr.write(u"[warn] %s\n" % w)
        if compare_src:
            rep = compare_with_source(out_path, compare_src, compare_dir or ".")
            sys.stderr.write(u"[compare] ok=%s（报告 %s/compare_report.json）\n"
                             % (rep["ok"], compare_dir or "."))
        return 0
    except EngineError as e:
        _cleanup(tmp_path)
        emit_failed(e.msg, e.errors)
        sys.stderr.write(u"[error] %s\n" % e.msg)
        for er in e.errors:
            sys.stderr.write(u"  - %s\n" % er)
        return 1
    except Exception as e:
        _cleanup(tmp_path)
        emit_failed(u"引擎内部错误：%s" % e)
        sys.stderr.write(traceback.format_exc())
        return 1


if __name__ == "__main__":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8")
    sys.exit(main(sys.argv[1:]))
