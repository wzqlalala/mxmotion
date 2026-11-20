#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""tdrrules.py——规则层加载（JSON+整行#注释剥离）与三层校验（python 3.7 兼容）。

规则层载体定型=JSON+注释剥离约定（03 §2.2，R3：捆绑 python 3.7.17 <3.11
无 tomllib 的降级案）：支持整行注释（行首空白+# 至行尾）按行剥除；不支持
行尾注释（杜绝字符串内 // 与 # 误剥）。

schema 定型来源=03-技术路线 §4.3（C3-06：H-05 键路径语法+H-12 per-kind
schema+V-04 caption 承载）与 04-评审意见-第3轮 第二节/第四节：
  - bind 为唯一绑定键名，bind_text=废弃别名（校验器兼容解析+warn）；
  - missing_policy 定型=字符串枚举（"dash_and_warn"/"keep_highlight"/"block_export"）；
  - ole policy≠preserve 首期=RuleError（C8：首期仅 preserve）；
  - caption 槽锚点=match_contains+seq_field_present+禁用 pStyle 过滤
    （htsy 首期不声明，schema 备而不用）；
  - expect 与 digest tables[] 回读事实交叉核对（S5-2 一并）。

三层校验（02 §2.3 步骤 4）：容器完整性（引擎/validate 工具负责 SHA1 对账，
本模块负责其后两层）→ 结构与定位（每槽声明经锚点+occurrence 唯一解析到
一处跨度，T-08/D-07）→ 引用一致性（键路径语法+数据字典定义+高亮全覆盖
契约）。校验器不保证语义正确性（D-18 人工检查单把关）。
"""

import json
import re
from typing import Any, Dict, List, Optional, Tuple

# 容器/规则格式版本（NFR-6 双版本；大版本不符拒载，小版本向后兼容）
FORMAT_VERSION = "1.0"
RULES_FORMAT_VERSION = "1.0"

# 高亮契约扫描范围声明（C3-08/H-08）：word/document.xml body 主文档流
HIGHLIGHT_CONTRACT_VALUES = ("all-must-declared",)

# missing_policy 字符串枚举（C3-06 定型，对象形态废止）
MISSING_POLICY_ENUM = ("dash_and_warn", "keep_highlight", "block_export")

# ole 处置态首期放行枚举（C8/H-12：policy≠preserve=RuleError）
OLE_POLICY_ENUM = ("preserve",)

# 图片尺寸策略（FR-24：fit_section_width=版心等比 / fit_cell=格宽等比）
IMAGE_SIZE_POLICY_ENUM = ("fit_section_width", "fit_cell")

# 表格行列伸缩语义（E2/E3）
TABLE_GROW_ENUM = ("copy_row_template",)
TABLE_SHRINK_ENUM = ("delete_extra_rows",)
TABLE_HEADER_POLICY_ENUM = ("keep_clear_highlight",)

# 循环章输出形态（U-05=A 每工况完整章，终审维持）
LOOP_OUTPUT_FORM_ENUM = ("full_chapter_per_case",)
LOOP_STATIC_ENUM_POLICY = ("keep_and_warn",)

# 键路径语法（C3-06/H-05 五作用域）：根/实例 case.{i}（块内写 case.*，
# 实例化按 instance_binding 改写）/循环块内全局共享/image 数组键/custom 豁免
ROOT_KEY_PREFIXES = ("model_stats.", "manual_fields.", "meta.", "conclusion.")
CASE_KEY_PREFIX = "case."
GLOBAL_SHARED_PREFIXES = ("materials.", "model_stats.", "manual_fields.", "meta.")
IMAGE_KEY_PREFIX = "image."
CUSTOM_KEY_PREFIX = "custom."


class RuleError(object):
    """FR-13 条目级定位三元组：entry（规则条目路径）/anchor（锚点字面）/
    reason（原因）。warn 级用 is_warn 标记（不计入失败）。"""

    def __init__(self, entry, anchor, reason, is_warn=False):
        # type: (str, str, str, bool) -> None
        self.entry = entry
        self.anchor = anchor
        self.reason = reason
        self.is_warn = is_warn

    def __str__(self):
        # type: () -> str
        # DX-04：highlight_coverage 条目的 anchor 字段承载跨度编号（非字面锚点），
        # 不再套"锚点="外壳（避免同字段两义误导排障）
        if self.entry == "highlight_coverage":
            return u"[%s] %s" % (self.entry, self.reason)
        return u"[%s] 锚点=%r：%s" % (self.entry, self.anchor, self.reason)

    def to_dict(self):
        # type: () -> Dict[str, Any]
        return {"entry": self.entry, "anchor": self.anchor,
                "reason": self.reason, "warn": self.is_warn}


def strip_line_comments(text):
    # type: (str) -> str
    """JSON+整行#注释剥离（03 §2.2）：行首空白+# 至行尾的整行剥除；
    不支持行尾注释。"""
    out = []
    for line in text.splitlines():
        if line.lstrip().startswith("#"):
            continue
        out.append(line)
    return "\n".join(out)


def load_rules(text):
    # type: (str) -> Dict[str, Any]
    """加载规则层：注释剥离→JSON 解析→schema 校验（结构/必填/枚举）。
    抛 ValueError（带文件内条目路径的原因）；返回规则 dict。
    CC-07：schema 硬伤（is_warn=False 条目）在此处即抛 ValueError——
    "JSON+schema 预检"对 tdr_rules put 等调用方真实生效（"不合格拒绝回填"），
    旧实现丢弃 _check_schema 结果致任意 JSON 对象都能回填成功。
    （文件+条目路径+原因 的 FR-13 定位由调用方在 ValueError 文案前缀补文件名。）"""
    clean = strip_line_comments(text)
    try:
        rules = json.loads(clean)
    except Exception as e:
        raise ValueError(u"规则层 JSON 解析失败：%s" % e)
    if not isinstance(rules, dict):
        raise ValueError(u"规则层顶层必须是 JSON 对象")
    errors = _check_schema(rules)
    hard = [e for e in errors if not e.is_warn]
    if hard:
        raise ValueError(u"规则校验失败 %d 条：%s"
                         % (len(hard), "；".join(str(e) for e in hard[:5])))
    return rules


def _err(entry, anchor, reason, is_warn=False):
    # type: (str, str, str, bool) -> RuleError
    return RuleError(entry, anchor, reason, is_warn)


def _check_schema(rules):
    # type: (Dict[str, Any]) -> List[RuleError]
    """per-kind schema 校验（03 §4.3 表）：必填字段/枚举/首期支持态。
    返回 errors+warns 混合列表；调用方按 is_warn 分流。"""
    errors = []   # type: List[RuleError]
    # L0 meta
    meta = rules.get("meta")
    if not isinstance(meta, dict):
        errors.append(_err("meta", "", u"缺少 meta 元信息对象"))
    else:
        fv = str(meta.get("format_version", ""))
        if fv.split(".")[0] != FORMAT_VERSION.split(".")[0]:
            errors.append(_err("meta.format_version", fv,
                               u"规则格式大版本不符（期望 %s.x，拒载）" % FORMAT_VERSION))
        if meta.get("highlight_contract") not in (None,) + HIGHLIGHT_CONTRACT_VALUES:
            errors.append(_err("meta.highlight_contract", str(meta.get("highlight_contract")),
                               u"高亮契约取值非法（仅支持 %s）" % (HIGHLIGHT_CONTRACT_VALUES,)))
    # L1 数据字典
    dd = rules.get("data_dictionary")
    if not isinstance(dd, dict):
        errors.append(_err("data_dictionary", "", u"缺少 L1 数据字典对象（键→类型）"))
    # L2 槽声明
    slots = rules.get("slots")
    if not isinstance(slots, list):
        errors.append(_err("slots", "", u"缺少 L2 槽声明数组"))
    else:
        for i, slot in enumerate(slots):
            errors.extend(_check_slot_schema(slot, "slots[%d]" % i))
    # L3 循环章
    loops = rules.get("loop_chapters", [])
    if not isinstance(loops, list):
        errors.append(_err("loop_chapters", "", u"loop_chapters 必须为数组"))
    else:
        for i, lp in enumerate(loops):
            errors.extend(_check_loop_schema(lp, "loop_chapters[%d]" % i))
    # keep_literals（原样保留声明：digest 候选未入槽的高亮跨度登记处）
    keeps = rules.get("keep_literals", [])
    if not isinstance(keeps, list):
        errors.append(_err("keep_literals", "", u"keep_literals 必须为数组"))
    else:
        for i, kp in enumerate(keeps):
            if not isinstance(kp, dict) or "match" not in (kp.get("anchor") or {}):
                errors.append(_err("keep_literals[%d]" % i, "",
                                   u"原样保留声明必须含 anchor.match"))
    # L4/L5
    fid = rules.get("fidelity")
    if not isinstance(fid, dict):
        errors.append(_err("fidelity", "", u"缺少 L4 保真声明对象"))
    exc = rules.get("exception_policy")
    if not isinstance(exc, dict):
        errors.append(_err("exception_policy", "", u"缺少 L5 异常策略对象"))
    else:
        if exc.get("missing_data") not in ("dash_and_warn", "keep_highlight", "block_export", None):
            errors.append(_err("exception_policy.missing_data", str(exc.get("missing_data")),
                               u"missing_data 枚举非法"))
    return errors


def _check_slot_schema(slot, entry):
    # type: (Dict[str, Any], str) -> List[RuleError]
    """单槽 per-kind schema（03 §4.3 表逐行）"""
    errors = []
    if not isinstance(slot, dict):
        return [_err(entry, "", u"槽声明必须是对象")]
    kind = slot.get("kind")
    anchor = slot.get("anchor")
    if not isinstance(anchor, dict):
        errors.append(_err(entry + ".anchor", "", u"槽声明缺少 anchor 对象"))
        anchor = {}
    # bind/bind_text 定型：统一 bind，bind_text=废弃别名（兼容解析+warn）
    if "bind_text" in slot and "bind" not in slot:
        slot = dict(slot)
        slot["bind"] = slot.pop("bind_text")
        errors.append(_err(entry + ".bind_text", str(anchor.get("match", "")),
                           u"bind_text 为废弃别名，已按 bind 解析（请迁移）", is_warn=True))
        slot.pop("bind_text", None)
    bind = slot.get("bind")
    if kind == "text":
        if not anchor.get("match"):
            errors.append(_err(entry + ".anchor.match", "", u"text 槽必须声明 match"))
        if "occurrence" not in anchor:
            errors.append(_err(entry + ".anchor.occurrence", str(anchor.get("match", "")),
                                   u"text 槽必须声明 occurrence（1 起/all）"))
        if not bind:
            errors.append(_err(entry + ".bind", str(anchor.get("match", "")),
                               u"text 槽必须声明 bind"))
    elif kind == "table":
        if not anchor.get("match") or anchor.get("element") != "table":
            errors.append(_err(entry + ".anchor", str(anchor.get("match", "")),
                               u"table 槽锚点必须为 {match, element='table'}"))
        cols = slot.get("columns")
        if not isinstance(cols, list) or not cols:
            errors.append(_err(entry + ".columns", str(anchor.get("match", "")),
                               u"table 槽必须声明非空 columns 数组"))
        else:
            for j, c in enumerate(cols):
                if not isinstance(c, dict) or "col" not in c:
                    errors.append(_err("%s.columns[%d]" % (entry, j), "",
                                       u"columns 元素必须含 col"))
        if slot.get("grow", "copy_row_template") not in TABLE_GROW_ENUM:
            errors.append(_err(entry + ".grow", str(slot.get("grow")),
                               u"grow 枚举非法（仅 %s）" % (TABLE_GROW_ENUM,)))
        if slot.get("shrink", "delete_extra_rows") not in TABLE_SHRINK_ENUM:
            errors.append(_err(entry + ".shrink", str(slot.get("shrink")),
                               u"shrink 枚举非法（仅 %s）" % (TABLE_SHRINK_ENUM,)))
        if slot.get("header_policy", "keep_clear_highlight") not in TABLE_HEADER_POLICY_ENUM:
            errors.append(_err(entry + ".header_policy", str(slot.get("header_policy")),
                               u"header_policy 枚举非法"))
        rg = slot.get("row_group")
        if rg is not None:
            if not isinstance(rg, dict) or "merge_col" not in rg or "restart_row" not in rg:
                errors.append(_err(entry + ".row_group", str(anchor.get("match", "")),
                                   u"row_group 必须含 merge_col/restart_row（行组语义）"))
            if not isinstance(slot.get("row_templates"), list):
                errors.append(_err(entry + ".row_templates", str(anchor.get("match", "")),
                                   u"行组槽必须声明 row_templates"))
    elif kind == "caption":
        # V-04/C3-01 R-c：锚点=match_contains+seq_field_present+禁用 pStyle 过滤
        if not anchor.get("match_contains"):
            errors.append(_err(entry + ".anchor.match_contains", "",
                               u"caption 槽必须声明 match_contains"))
        if "seq_field_present" not in anchor:
            errors.append(_err(entry + ".anchor.seq_field_present", str(anchor.get("match_contains", "")),
                               u"caption 槽必须声明 seq_field_present"))
        if "pStyle" in anchor:
            errors.append(_err(entry + ".anchor.pStyle", "",
                               u"题注定位禁用样式过滤（实测题注段 pStyle=af2/无 pStyle）"))
    elif kind == "image":
        # 三形态锚点（T-08/D-10）：占位段落文本 / 空段落坐标 / 表格坐标
        form = _image_anchor_form(anchor)
        if form is None:
            errors.append(_err(entry + ".anchor", "",
                               u"image 槽锚点必须是三形态之一（match / "
                               u"element=block_paragraph+block / element=table+table_match+cell）"))
        if not bind:
            errors.append(_err(entry + ".bind", "", u"image 槽必须声明 bind（image.<key>.path）"))
        size = slot.get("size")
        if not isinstance(size, dict) or size.get("policy") not in IMAGE_SIZE_POLICY_ENUM:
            errors.append(_err(entry + ".size.policy", str(size),
                               u"image 槽必须声明 size.policy（%s）" % (IMAGE_SIZE_POLICY_ENUM,)))
        mp = slot.get("missing_policy", "dash_and_warn")
        if mp not in MISSING_POLICY_ENUM:
            errors.append(_err(entry + ".missing_policy", str(mp),
                               u"missing_policy 必须为字符串枚举 %s"
                               % (MISSING_POLICY_ENUM,)))
    elif kind == "conditional_text":
        if not anchor.get("match_prefix"):
            errors.append(_err(entry + ".anchor.match_prefix", "", u"条件措辞槽必须声明 match_prefix"))
        if not isinstance(slot.get("branches"), list):
            errors.append(_err(entry + ".branches", str(anchor.get("match_prefix", "")),
                               u"条件措辞槽必须声明 branches 数组"))
        if not bind:
            errors.append(_err(entry + ".bind", str(anchor.get("match_prefix", "")),
                               u"条件措辞槽必须声明 bind（判定键）"))
    elif kind == "ole":
        if not anchor.get("ole_prog_id"):
            errors.append(_err(entry + ".anchor.ole_prog_id", "", u"ole 槽必须声明 ole_prog_id"))
        if slot.get("policy") not in OLE_POLICY_ENUM:
            errors.append(_err(entry + ".policy", str(slot.get("policy")),
                               u"ole policy 首期仅支持 preserve（%s 等类型列后续版本，校验即错）"
                               % (slot.get("policy"),)))
    else:
        errors.append(_err(entry + ".kind", str(kind),
                           u"槽类型非法（可选：text/table/caption/image/conditional_text/ole；"
                           u"循环章为独立声明，不属于槽类型）"))
    return errors


def _image_anchor_form(anchor):
    # type: (Dict[str, Any]) -> Optional[str]
    if anchor.get("match"):
        return "text"
    if anchor.get("element") == "block_paragraph" and anchor.get("block"):
        return "block_paragraph"
    if anchor.get("element") == "table" and anchor.get("table_match") and anchor.get("cell"):
        return "table_cell"
    return None


def _check_loop_schema(lp, entry):
    # type: (Dict[str, Any], str) -> List[RuleError]
    errors = []
    tr = lp.get("template_range")
    if not isinstance(tr, dict) or not tr.get("from_heading") or not tr.get("to_before_heading"):
        errors.append(_err(entry + ".template_range", "",
                           u"循环章必须声明 from_heading/to_before_heading"))
    if lp.get("output_form", "full_chapter_per_case") not in LOOP_OUTPUT_FORM_ENUM:
        errors.append(_err(entry + ".output_form", str(lp.get("output_form")),
                           u"output_form 枚举非法（仅 full_chapter_per_case）"))
    rn = lp.get("renumber")
    if not isinstance(rn, dict):
        errors.append(_err(entry + ".renumber", "", u"循环章必须声明 renumber"))
    else:
        # H-06 显式公式：heading 必须=literal:K_i=5+i；subsection=literal:K_i.{1..k}
        if rn.get("heading") != "literal:K_i=5+i":
            errors.append(_err(entry + ".renumber.heading", str(rn.get("heading")),
                               u"章号公式必须为 literal:K_i=5+i（显式公式口径）"))
        sub = rn.get("subsection", "")
        if not re.match(r"^literal:K_i\.\{\d+(,\d+)*\}$", str(sub)):
            errors.append(_err(entry + ".renumber.subsection", str(sub),
                               u"子节公式必须为 literal:K_i.{1,2,3} 形态（连续重排口径）"))
    if lp.get("static_enum_policy", "keep_and_warn") not in LOOP_STATIC_ENUM_POLICY:
        errors.append(_err(entry + ".static_enum_policy", str(lp.get("static_enum_policy")),
                           u"static_enum_policy 枚举非法"))
    lr = lp.get("literal_rewrite", [])
    if not isinstance(lr, list):
        errors.append(_err(entry + ".literal_rewrite", "", u"literal_rewrite 必须为数组"))
    else:
        for j, it in enumerate(lr):
            if not isinstance(it, dict) or not it.get("match") or not it.get("rewrite_to"):
                errors.append(_err("%s.literal_rewrite[%d]" % (entry, j), "",
                                   u"literal_rewrite 元素必须含 match/rewrite_to"))
    if not lp.get("instance_key"):
        errors.append(_err(entry + ".instance_key", "", u"循环章必须声明 instance_key（如 dataset.cases）"))
    if not lp.get("instance_binding"):
        errors.append(_err(entry + ".instance_binding", "", u"循环章必须声明 instance_binding（如 case.{i}）"))
    return errors


# ---------------------------------------------------------------------------
# 三层校验之结构定位与引用一致性（容器完整性由引擎 M1/validate 工具负责）
# ---------------------------------------------------------------------------

def _anchor_phrase(slot):
    # type: (Dict[str, Any]) -> str
    a = slot.get("anchor") or {}
    return a.get("match") or a.get("match_contains") or a.get("match_prefix") or ""


def resolve_anchor(rules, index):
    # type: (Dict[str, Any], Dict[str, Any]) -> List[RuleError]
    """结构与定位层：每槽声明经锚点+occurrence 唯一解析（T-08/D-07）。
    index 为 digest 产出的 assets/content/index.json 数据（anchors/tables/
    blocks/fields）。occurrence 语义：整数 k=同字面第 k 处（1 起）；
    "all"=全部同字面处。表格锚点解析=题注字面块→其后首个表格块（htsy 实测：
    "计算载荷"字面在题注段，表格紧随其后）。"""
    errors = []
    anchors = index.get("anchors", [])
    tables = index.get("tables", [])
    by_text = {}   # type: Dict[str, List[Dict[str, Any]]]
    for a in anchors:
        by_text.setdefault(a.get("text", ""), []).append(a)
    slots = rules.get("slots", [])
    for i, slot in enumerate(slots):
        entry = "slots[%d]" % i
        kind = slot.get("kind")
        anchor = slot.get("anchor") or {}
        phrase = _anchor_phrase(slot)
        if kind == "table":
            hit = _resolve_table_entries(index, phrase)
            if not hit:
                errors.append(_err(entry + ".anchor.match", phrase, u"表格锚点未命中任何表"))
            else:
                errors.extend(_expect_crosscheck(entry, slot, hit[0]))
            continue
        if kind == "ole":
            ole_list = index.get("fields", {}).get("ole_objects", [])
            n = len(ole_list)
            occ = anchor.get("occurrence", 1)
            if n == 0:
                errors.append(_err(entry + ".anchor.ole_prog_id", str(anchor.get("ole_prog_id")),
                                   u"容器内未发现 OLE 对象"))
            elif isinstance(occ, int) and occ > n:
                errors.append(_err(entry + ".anchor.occurrence", str(occ),
                                   u"OLE 对象仅 %d 处，occurrence 解析不唯一" % n))
            continue
        if kind == "image" and anchor.get("element") == "table":
            hit = _resolve_table_entries(index, str(anchor.get("table_match", "")))
            if not hit:
                errors.append(_err(entry + ".anchor.table_match", str(anchor.get("table_match")),
                                   u"图片槽表格坐标锚点未命中任何表"))
            else:
                cell = str(anchor.get("cell", ""))
                m = re.match(r"^r(\d+)c(\d+)$", cell)
                t0 = hit[0]
                if not m or int(m.group(1)) >= t0.get("rows", 0) or int(m.group(2)) >= t0.get("cols", 0):
                    errors.append(_err(entry + ".anchor.cell", cell,
                                       u"单元格坐标越界（表 %drows×%dcols）"
                                       % (t0.get("rows", 0), t0.get("cols", 0))))
            continue
        if kind == "image" and anchor.get("element") == "block_paragraph":
            blk = str(anchor.get("block", ""))
            if not _block_exists(index, blk):
                errors.append(_err(entry + ".anchor.block", blk, u"空段落坐标块不存在"))
            continue
        if kind == "conditional_text":
            # 整句措辞槽：块文本前缀唯一命中（锚点=段落级，非高亮跨度级）
            pool = [b for b in index.get("blocks", [])
                    if phrase and (b.get("text") or "").strip().startswith(phrase)]
            if len(pool) != 1:
                errors.append(_err(entry + ".anchor.match_prefix", phrase,
                                   u"前缀命中 %d 个块（须唯一）" % len(pool)))
            continue
        # 文本/题注：字面定位
        if kind == "caption":
            cand = [a for a in anchors if phrase and phrase in a.get("text", "")]
            seq_ok = [a for a in cand if a.get("has_seq_field")]
            pool = seq_ok if anchor.get("seq_field_present") else cand
        else:
            pool = by_text.get(phrase, [])
            if not pool:
                # 兼容 match_contains（题注/变体锚点字面）
                pool = [a for a in anchors if phrase and phrase in a.get("text", "")]
        if not pool:
            errors.append(_err(entry + ".anchor", phrase, u"锚点未命中任何归并跨度"))
            continue
        occ = anchor.get("occurrence", 1)
        if occ == "all":
            if len(pool) < 1:
                errors.append(_err(entry + ".anchor.occurrence", "all", u"无同字面跨度"))
        elif isinstance(occ, int) and 1 <= occ <= len(pool):
            pass
        else:
            errors.append(_err(entry + ".anchor.occurrence", str(occ),
                               u"同字面跨度共 %d 处，occurrence 解析不唯一" % len(pool)))
    return errors


def _expect_crosscheck(entry, slot, table_fact):
    # type: (str, Dict[str, Any], Dict[str, Any]) -> List[RuleError]
    """expect 与 digest tables[] 回读事实交叉核对（H-12/S5-2：rows/cols 不符=错）"""
    errors = []
    expect = str((slot.get("anchor") or {}).get("expect", ""))
    m = re.match(r"^(\d+)rows:(\d+)cols", expect)
    if m:
        if int(m.group(1)) != table_fact.get("rows") or int(m.group(2)) != table_fact.get("cols"):
            errors.append(_err(entry + ".anchor.expect", expect,
                               u"与 digest 表格回读事实不符（实测 %drows:%dcols）"
                               % (table_fact.get("rows", 0), table_fact.get("cols", 0))))
    # S5-2 轻量语义探测：columns 键 shape vs 表格列数
    for c in slot.get("columns", []) or []:
        col = c.get("col")
        if isinstance(col, int) and col >= table_fact.get("cols", 0):
            errors.append(_err(entry + ".columns", str(col),
                               u"列号 %d 超出表格列数 %d" % (col, table_fact.get("cols", 0))))
    rg = slot.get("row_group")
    if rg is not None:
        rr = int(rg.get("restart_row", 1))
        if rr >= table_fact.get("rows", 0):
            errors.append(_err(entry + ".row_group.restart_row", str(rr),
                               u"行组 restart 行号超出表格行数"))
    return errors


def _resolve_table_entries(index, phrase):
    # type: (Dict[str, Any], str) -> List[Dict[str, Any]]
    """表格槽锚点解析：字面命中块（题注段）→ 文档序其后首个表格块。
    兼容字面直接在表内的情形（表块文本命中即返回该表）。"""
    if not phrase:
        return []
    blocks = index.get("blocks", [])
    tables = index.get("tables", [])
    # 表块文本直接命中
    for t in tables:
        if phrase in (t.get("text") or ""):
            return [t]
    # 题注块命中 → 其后首个表
    hit_orders = [i for i, b in enumerate(blocks) if phrase in (b.get("text") or "")]
    if not hit_orders:
        return []
    start = hit_orders[0]
    t_by_order = {t.get("order"): t for t in tables}
    for i in range(start + 1, len(blocks)):
        if i in t_by_order:
            return [t_by_order[i]]
    return []


def _blocks_containing(index, phrase):
    # type: (Dict[str, Any], str) -> List[Dict[str, Any]]
    if not phrase:
        return []
    return [b for b in index.get("blocks", []) if phrase in (b.get("text") or "")]


def _block_exists(index, blk):
    # type: (Dict[str, Any], str) -> bool
    return any(b.get("id") == blk for b in index.get("blocks", []))


def check_references(rules, dataset, index):
    # type: (Dict[str, Any], Optional[Dict[str, Any]], Dict[str, Any]) -> List[RuleError]
    """引用一致性层：bind 键路径语法（C3-06 五作用域）+数据字典定义核对；
    custom.* 豁免字典定义（仅查数据集存在性，NFR-5）；数据集给定时附带
    存在性核对（S5-2 instance_key 数组存在性）。"""
    errors = []
    dd = rules.get("data_dictionary", {})
    defined = set(dd.keys())
    slots = rules.get("slots", [])
    for i, slot in enumerate(slots):
        entry = "slots[%d]" % i
        kind = slot.get("kind")
        phrase = _anchor_phrase(slot)
        if kind == "table":
            for j, c in enumerate(slot.get("columns", [])):
                b = c.get("bind")
                if b and b != "keep_literal":
                    errors.extend(_check_bind(entry + ".columns[%d].bind" % j, b,
                                              defined, dd, phrase))
            continue
        b = slot.get("bind")
        if not b:
            continue
        if kind == "image":
            if not re.match(r"^image\.[A-Za-z0-9_]+\.(path|caption)$", str(b)):
                errors.append(_err(entry + ".bind", str(b),
                                   u"image 槽 bind 必须为 image.<key>.(path|caption) 形态"))
            continue
        errors.extend(_check_bind(entry + ".bind", b, defined, dd, phrase))
    # 循环章 instance_key 存在性（S5-2）
    for i, lp in enumerate(rules.get("loop_chapters", [])):
        ikey = str(lp.get("instance_key", ""))
        leaf = ikey.split(".", 1)[-1] if "." in ikey else ikey
        if dataset is not None and leaf not in dataset:
            errors.append(_err("loop_chapters[%d].instance_key" % i, ikey,
                               u"数据集缺少实例数组 %r" % leaf))
        # 分支文案 {key} 引用核对（H-14/C12-Q-01 裁定 b：全限定键单一语法，
        # 与 C3-06 键路径语法一致；BUG-01/BUG-03 同点修复）
    for i, slot in enumerate(rules.get("slots", [])):
        if slot.get("kind") != "conditional_text":
            continue
        for j, br in enumerate(slot.get("branches", []) or []):
            entry = "slots[%d].branches[%d]" % (i, j)
            use = str(br.get("use", ""))
            texts = []   # type: List[str]
            if use and dataset is not None:
                # BUG-03：use 带作用域前缀（conclusion.pass）按全路径逐段下钻
                # 查叶子键（旧逻辑把 "conclusion.pass" 整串与 conclusion 叶子
                # 键比对→样例数据集恒误报 warn）；裸叶子键（无点）与引擎
                # render_conditional 的 conclusion.<use> 回退同构，查 conclusion 下
                segs = use.split(".")
                if len(segs) == 1:
                    root_node = dataset.get("conclusion")
                    node = root_node.get(use) if isinstance(root_node, dict) else None
                    root = "conclusion"
                else:
                    root = segs[0]
                    node = dataset.get(segs[0])
                    for seg in segs[1:]:
                        if not isinstance(node, dict) or seg not in node:
                            node = None
                            break
                        node = node[seg]
                if node is None:
                    errors.append(_err(entry, use,
                                       u"分支文案键 %r 在数据集 %s 中不存在"
                                       % (use, root), is_warn=True))
                elif isinstance(node, str):
                    texts.append(node)   # use 间接文案一并扫描（C12：{键} 不得逃逸）
            inline = str(br.get("text", ""))
            if inline:
                texts.append(inline)   # 规则内联分支文案一并扫描
            for text in texts:
                # BUG-01（C12 裁定 b）：裸键/未定义键引用=条目级错误（is_warn=False），
                # 提炼/校验期阻断，不产出字面残留输出件
                for ref in re.findall(r"\{([A-Za-z0-9_.]+)\}", text):
                    errors.extend(_check_branch_text_ref(entry, ref, defined, dd))
    return errors


def _check_branch_text_ref(entry, ref, defined, dd):
    # type: (str, str, set, Dict[str, Any]) -> List[RuleError]
    """分支文案 {键} 引用核对（BUG-01/C12-Q-01：一律全限定键形态——与
    C3-06 键路径语法单一；裸键/未定义键=条目级错误校验期阻断；
    custom.* 豁免（NFR-5））。"""
    b = str(ref)
    if b.startswith(CUSTOM_KEY_PREFIX):
        return []
    if "." not in b:
        return [_err(entry, b, u"分支文案键引用 %r 为裸键（C12-Q-01：须全限定，"
                               u"如 {{manual_fields.project_subject}}）" % b)]
    if b.startswith(IMAGE_KEY_PREFIX):
        if not re.match(r"^image\.[A-Za-z0-9_]+\.(path|caption)$", b):
            return [_err(entry, b, u"分支文案引用键 %r 不符合 image.<key>.(path|caption) 形态" % b)]
        return []
    return _check_bind(entry, b, defined, dd, "")


def _check_bind(entry, bind, defined, dd, phrase):
    # type: (str, str, set, Dict[str, Any], str) -> List[RuleError]
    """单 bind 键路径核对：根作用域前缀+字典定义（custom.* 豁免；case.* 为
    实例作用域——校验其叶子路径在数据字典有 case.* 模板或按前缀放行）。"""
    b = str(bind)
    if b == "keep_literal":
        return []
    if b.startswith(CUSTOM_KEY_PREFIX):
        return []   # NFR-5 客户扩展键豁免字典定义
    if b.startswith(CASE_KEY_PREFIX):
        # 实例作用域：字典须有同前缀模板键（case.*）或精确键
        ok = b in defined or _dict_has_case_path(dd, b)
        if not ok:
            return [_err(entry, b, u"实例键 %r 未在数据字典定义（case.* 模板）" % b)]
        return []
    if b.startswith(GLOBAL_SHARED_PREFIXES) or b.startswith(ROOT_KEY_PREFIXES):
        if b not in defined:
            return [_err(entry, b, u"键 %r 未在数据字典定义" % b)]
        return []
    return [_err(entry, b, u"键路径 %r 不符合五作用域语法（根/case.{i}/materials 等"
                           u"全局共享/image.*/custom.*）" % b)]


def _dict_has_case_path(dd, bind):
    # type: (Dict[str, Any], str) -> bool
    """数据字典存在 case.* 模板键且其路径覆盖 bind（如 case.loads.position
    由模板键 case.loads.* 或逐键定义覆盖）"""
    for k in dd.keys():
        if not k.startswith(CASE_KEY_PREFIX):
            continue
        if k == bind:
            return True
        if k.endswith(".*") and bind.startswith(k[:-1]):
            return True
    return False


def check_highlight_coverage(rules, index):
    # type: (Dict[str, Any], Dict[str, Any]) -> List[RuleError]
    """高亮全覆盖契约（02 §2.1.3-4/FR-11）：扫描范围=body 主文档流（C3-08）。
    每个 highlight 跨度必须：被某槽声明解析命中 / keep_literals 原样保留 /
    引擎托管（in_field_result=SEQ 缓存值[S5-4]、循环题注域后子跨度[R3-01]）。
    未声明=错误；条目=hl 编号+短语。"""
    errors = []
    anchors = index.get("anchors", [])
    declared_texts = set()
    for slot in rules.get("slots", []):
        phrase = _anchor_phrase(slot)
        if phrase:
            declared_texts.add(phrase)
        # 表格槽覆盖其所在表块内全部跨度；图片槽覆盖 caption_cell/锚点跨度
    table_blocks = set()
    for slot in rules.get("slots", []):
        # 表格槽/图片槽表格坐标锚点：解析=题注字面块→其后首个表格块
        # （云图 A~D 的 caption_cell 子题注跨度随目标表块覆盖）
        if slot.get("kind") in ("table", "image"):
            a = slot.get("anchor") or {}
            phrase = a.get("match") or a.get("table_match") or ""
            for t in _resolve_table_entries(index, phrase):
                table_blocks.add(t.get("block"))
    keep_texts = set()
    for kp in rules.get("keep_literals", []):
        keep_texts.add(str((kp.get("anchor") or {}).get("match", "")))
    span_no = 0
    for a in anchors:
        if not a.get("is_highlight_span"):
            continue
        span_no += 1
        hid = a.get("id", "hl_?")
        text = a.get("text", "")
        blk = a.get("block")
        if a.get("in_field_result"):
            continue   # S5-4：SEQ 缓存值，引擎托管
        if a.get("managed") == "engine_caption":
            continue   # R3-01：循环题注域后子跨度，实例化语义隐式承接
        if text in declared_texts or text in keep_texts or blk in table_blocks:
            continue
        errors.append(_err("highlight_coverage", hid,
                           u"高亮跨度 %s（'%s' @%s）未声明：请为该短语补一条槽声明"
                           u"（bind），或声明为原样保留（keep_literals）/表格槽覆盖"
                           % (hid, text, blk)))
    return errors


def validate_rules(rules, index, dataset=None):
    # type: (Dict[str, Any], Dict[str, Any], Optional[Dict[str, Any]]) -> List[RuleError]
    """三层校验入口（容器完整性层由调用方先行）：返回 RuleError 列表，
    含 warn 级（is_warn=True，不计失败）。"""
    errors = _check_schema(rules)
    if any(not e.is_warn for e in errors):
        return errors   # schema 硬伤时不再继续（条目级信息已足）
    errors += resolve_anchor(rules, index)
    errors += check_references(rules, dataset, index)
    errors += check_highlight_coverage(rules, index)
    return errors


def errors_summary(errors):
    # type: (List[RuleError]) -> Dict[str, Any]
    return {
        "error_count": sum(1 for e in errors if not e.is_warn),
        "warn_count": sum(1 for e in errors if e.is_warn),
        "items": [e.to_dict() for e in errors],
    }
