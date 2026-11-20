#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""tdr_digest.py——消化工具：源 docx → 参数化规则模板文档（.rtdoc）+ 消化报告。

提炼期一次性工具（与宿主零耦合）。主流程（03-技术路线 §4.2，逐节落实）：
  ⓪ E6 前置校验（扩展名/试解包/占用探测，C2；REF-01/C15 占用探测放宽为
    读模式试开——共享可读即消化并追加编辑中提示，真不可读才阻断）——
    校验不过=可读原因+非 0 退出码+不产出任何文件；
  ① 资产提取（T-01 部件闭包：默认去向表+manifest 对账 30 部件全有去向）；
  ② 正文切块（body 顶层片段+域/书签配对栈并块，边界永不落在域/书签对内部，
    B1 并块优先级高于章边界与表格整表块；块内容=源 XML 字节原样）；
  ③ 跨 run 高亮归并（R-a 域硬边界：fldChar/instrText=硬边界，跨度拆域前/
    域结果[in_field_result=引擎托管]/域后子跨度——C3-01/H-01 修法）；
  ④ 占位扫描候选（hl_001… 编号+occurrence+建议键名，T-08/T-05；空括号占位
    专项建议 displacement_max；S5-4：SEQ 缓存子跨度自动标注"引擎托管"）；
  ⑤ 非高亮需处置字面报告（D-10/D-24 六项：死子节/空段图片位/措辞候选/
    静态枚举/VML 文本框/页眉页脚封面残留——C3-04 扩页眉页脚扫描）；
  ⑥ 域清单/书签清单（R5：38 fldChar/28 书签逐一登记；_GoBack 单列 H-11；
    PAGEREF _Toc 死条目登记"刷新自愈" H-10/C3-08）；
  ⑦ B6 跨章 REF 防御检测 + C-11 循环块 w:pict warn + S5-1 模板特性清单 warn；
  ⑧ 规则层候选草稿生成（六层结构，02 §2.2；人工确认检查单 D-18 把关语义）。

子命令：
  <src.docx> <out_dir> [--suggest <path>]        消化（默认；--suggest 显式指定
                                                 建议表伴生文件 T0a，缺省=源 docx
                                                 同目录同名 XXX.suggest.jsonc
                                                 自动发现，无伴生=内置表）
  verify_roundtrip <src.docx> <container>       提炼产物基准①比对（FR-31①）
  upgrade_container <src.docx> <old> <out_dir>  R11 重消化+新旧 rules diff

工程约束：python 3.7 兼容（typing.Dict/List/Tuple；open 显式 encoding）；
源文件全程只读、从不写回（FR-12）；跨目录复用=入口 sys.path 注入
<脚本根>/../tdr_engine（B5/T-02-a：源树与发布包两形态同构）。
"""

import io
import json
import hashlib
import os
import re
import sys
import zipfile
from typing import Any, Dict, List, Optional, Tuple

sys.path.insert(0, os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "tdr_engine")))

import tdroox  # noqa: E402
import tdrrules  # noqa: E402

# manifest 输入上限默认值（S5-3/H-16：超限 M1 拒载；digest 侧写入 manifest）
INPUT_LIMITS = {
    "max_parts": 200,            # 部件数上限
    "max_part_bytes": 52428800,  # 单部件 50MB
    "max_total_bytes": 524288000,  # 容器解压总体积 500MB
    "max_ratio": 200,            # 解压比率（解压后/压缩后）上限
}

# 资产化部件的容器内落位映射（T-01 默认去向表；逐字节资产）
_ASSET_MAP = [
    (r"^word/styles\.xml$", "assets/styles.xml"),
    (r"^word/numbering\.xml$", "assets/numbering.xml"),
    (r"^word/settings\.xml$", "assets/settings.xml"),
    (r"^word/fontTable\.xml$", "assets/fontTable.xml"),
    (r"^word/webSettings\.xml$", "assets/webSettings.xml"),
    (r"^word/footnotes\.xml$", "assets/footnotes.xml"),
    (r"^word/endnotes\.xml$", "assets/endnotes.xml"),
    (r"^word/customizations\.xml$", "assets/customizations.xml"),
    (r"^word/attachedToolbars\.bin$", "assets/attachedToolbars.bin"),
    (r"^word/_rels/customizations\.xml\.rels$", "assets/rels/customizations.xml.rels"),
    (r"^word/_rels/document\.xml\.rels$", "assets/rels/document.xml.rels"),
    (r"^word/theme/(.+)$", r"assets/theme/\1"),
    (r"^word/media/(.+)$", r"assets/media/\1"),
    (r"^word/embeddings/(.+)$", r"assets/media/embeddings/\1"),
    (r"^word/header(\d+)\.xml$", r"assets/headers/header\1.xml"),
    (r"^word/footer(\d+)\.xml$", r"assets/footers/footer\1.xml"),
    (r"^customXml/_rels/(.+)$", r"assets/customXml/_rels/\1"),
    (r"^customXml/(.+)$", r"assets/customXml/\1"),
    (r"^docProps/(.+)$", r"assets/docProps/\1"),
]


def sha1_bytes(data):
    # type: (bytes) -> str
    return hashlib.sha1(data).hexdigest()


def asset_store_name(source_part):
    # type: (str) -> Optional[str]
    for pat, repl in _ASSET_MAP:
        m = re.match(pat, source_part)
        if m:
            return re.sub(pat, repl, source_part)
    return None


class DigestBlocked(Exception):
    """E6 前置校验阻断（可读原因，非 0 退出码，不产出任何文件）"""

    def __init__(self, reason):
        # type: (str) -> None
        super(DigestBlocked, self).__init__(reason)
        self.reason = reason


# ---------------------------------------------------------------------------
# ⓪ E6 提炼输入前置校验（03 §4.2-⓪，C2：三检查全过才进资产提取）
# ---------------------------------------------------------------------------

def verify_input(src_docx_path):
    # type: (str) -> Tuple[zipfile.ZipFile, bool]
    """扩展名→读试开→占用探测；失败抛 DigestBlocked（可读原因）。
    校验通过后源文件全程只读（消化工具从不写回，FR-12）。
    REF-01（C15）占用探测放宽：读模式试开（"rb"）为口径——共享可读即放行，
    返回值第二项=是否"可读但存在写锁/只读属性"（True 时由 digest 在消化报告
    追加编辑中提示）；rb 仍 PermissionError=真不可读（独占锁或权限限制）→阻断。"""
    if not os.path.isfile(src_docx_path):
        raise DigestBlocked(u"源文件不存在：%s" % src_docx_path)
    ext = os.path.splitext(src_docx_path)[1].lower()
    if ext == ".doc":
        raise DigestBlocked(
            u"源文件为 .doc 老格式：请先在 Word 中另存为 .docx 后重试（%s）" % src_docx_path)
    if ext != ".docx":
        # R3X-05①：扩展名展示统一带前导点+直引号（与向导步① GUI 文案逐字一致——
        # FR-251"E5/E6 负例文案与 CLI 一致"口径；原 %r 为 '.txt' repr 单引号形态）
        raise DigestBlocked(
            u"扩展名必须为 .docx（当前 \"%s\"）：请确认文件格式，.doc 请先另存为 .docx" % ext)
    # 占用探测（REF-01/C15：读模式试开——Word 打开 docx 只加写锁、允许共享读，
    # 此前写模式试开把"可安全读"场景误判阻断，与 FR-12 只读不写回相悖；
    # 只读属性文件现在可正常消化）
    try:
        with open(src_docx_path, "rb"):
            pass
    except PermissionError:
        # 独占锁/权限限制下连共享读也不可得——真正不可读，阻断（不再提
        # "解除占用/只读"：只读属性文件已在上一步试开中放行）
        raise DigestBlocked(u"源文件不可读（独占锁或权限限制），请检查后重试")
    except OSError as e:
        raise DigestBlocked(u"源文件不可读：%s" % e)
    src_write_locked = False
    try:
        with open(src_docx_path, "r+b"):
            pass
    except (PermissionError, OSError):
        # 可读但写不开=被 Word 等以写锁占用（共享读）或只读属性——不阻断，
        # digest 按磁盘最近保存版本消化并在报告追加提示
        src_write_locked = True
    # 试解包（伪 zip/加密 OOXML=CFB 容器/损坏 → 阻断）
    try:
        z = zipfile.ZipFile(src_docx_path, "r")
    except Exception as e:
        raise DigestBlocked(u"文件损坏或非 docx 格式（无法按 zip 解包）：%s" % e)
    names = set(z.namelist())
    missing = [n for n in ("[Content_Types].xml", "word/document.xml") if n not in names]
    if missing:
        z.close()
        raise DigestBlocked(u"文件损坏或非 docx 格式（缺少部件 %s）" % ", ".join(missing))
    return z, src_write_locked


# ---------------------------------------------------------------------------
# ② 正文切块：配对栈并块（域/书签边界永不切穿，B1 优先级）
# ---------------------------------------------------------------------------

class Block(object):
    """内容块：1..n 个顶层片段的字节原样聚合。

    kind：para/table/sectpr/fieldgroup（多片段并块）；heading：块内首个
    章级标题段文本（chapter）/子节标题文本（subsection）；chapter：章序号
    （首个章标题前=0）。"""

    def __init__(self, blk_id):
        # type: (str) -> None
        self.id = blk_id                  # type: str
        self.frags = []                   # type: List[Tuple[str, str]]  # (tag, xml)
        self.kind = "para"                # type: str
        self.style = ""                   # type: str
        self.is_heading = False           # type: bool   # styleId=1 章级
        self.is_subsection = False        # type: bool   # af7 子节
        self.heading_text = ""            # type: str
        self.chapter = 0                  # type: int
        self.text = ""                    # type: str

    def xml(self):
        # type: () -> str
        return "".join(x for _, x in self.frags)


def split_blocks(body):
    # type: (str) -> Tuple[List[Block], List[str]]
    """body 顶层片段线性扫描 → 块列表。配对栈规则（03 §4.2-①）：
    处于未闭合域对/书签对内部的片段并入当前块；并块吞并章标题段或表格时
    记 warn（B1：不静默）。返回 (blocks, warns)。"""
    warns = []
    frags = tdroox.top_level_children(body)
    blocks = []   # type: List[Block]
    cur = None    # type: Optional[Block]
    fld_depth = 0
    bm_depth = 0
    for fi, (tag, a, b) in enumerate(frags):
        frag = body[a:b]
        # CC-04：域/书签配对计数在掩码副本上进行（注释/CDATA 内伪 fldChar/
        # bookmark 不污染配对栈）；块内容仍存原文 frag
        frag_m = tdroox.mask_opaque(frag)
        fb = len(re.findall(r'<w:fldChar w:fldCharType="begin"', frag_m))
        fe = len(re.findall(r'<w:fldChar w:fldCharType="end"', frag_m))
        bs = len(re.findall(r"<w:bookmarkStart ", frag_m))
        be = len(re.findall(r"<w:bookmarkEnd ", frag_m))
        merged = (fld_depth > 0 or bm_depth > 0) and cur is not None
        if merged:
            if tag == "w:tbl":
                warns.append(u"配对栈并块吞并表格片段 #%d（B1 并块优先级，"
                             u"块 %s 整表语义降级为域组）" % (fi, cur.id))
            cur.frags.append((tag, frag))
            cur.kind = "fieldgroup"
        else:
            cur = Block("blk_%03d" % len(blocks))
            cur.frags.append((tag, frag))
            blocks.append(cur)
        fld_depth += fb - fe
        bm_depth += bs - be
        if fld_depth < 0 or bm_depth < 0:
            warns.append(u"片段 #%d 出现未配对的域/书签闭合标签（线性扫描深度<0）" % fi)
            fld_depth = max(fld_depth, 0)
            bm_depth = max(bm_depth, 0)
    # 块元信息：kind/style/heading/text
    for blk in blocks:
        xml = blk.xml()
        if len(blk.frags) > 1:
            blk.kind = "fieldgroup"
        elif blk.frags[0][0] == "w:tbl":
            blk.kind = "table"
        elif blk.frags[0][0] == "w:sectPr":
            blk.kind = "sectpr"
        else:
            blk.kind = "para"
        sm = re.search(r'<w:pStyle w:val="([^"]+)"/>', xml)
        blk.style = sm.group(1) if sm else ""
        blk.text = tdroox.fragment_text(xml)
        if blk.kind in ("para", "fieldgroup"):
            if blk.style == "1":
                blk.is_heading = True
                blk.heading_text = blk.text.strip()
            elif blk.style == "af7":
                blk.is_subsection = True
                blk.heading_text = blk.text.strip()
    # 章序号：章级标题段所在块起新章；首章标题前=0
    chapter = 0
    for blk in blocks:
        if blk.is_heading:
            chapter += 1
        blk.chapter = chapter
    return blocks, warns


# ---------------------------------------------------------------------------
# ③ 跨 run 高亮归并（R-a 域硬边界）+ 占位扫描
# ---------------------------------------------------------------------------

def scan_block_spans(blk):
    # type: (Block) -> Dict[str, Any]
    """块内高亮扫描：逐"文本单元"（para 块=整块；table 块=逐单元格逐段）
    归并高亮跨度（tdroox.merge_highlight_spans 的 R-a 域硬边界语义），
    同时归类空 w:t 高亮 run 与段落标记高亮（128 处高亮会计学，03 §0）。
    返回 {spans:[…], hl_text_runs, hl_empty_runs, hl_para_marks, hl_break_runs,
    has_seq_span}"""
    spans = []                 # type: List[Dict[str, Any]]
    hl_empty = 0
    hl_field = 0
    hl_mark = 0
    hl_break = 0               # CA-04②：高亮 text+br run（不参与跨度归并，单独计数）
    units = []                 # type: List[Tuple[str, str]]  # (cell, para_xml)
    xml = blk.xml()
    if blk.kind == "table":
        for ra, rb in tdroox.child_spans(xml, "w:tr"):
            row = xml[ra:rb]
            for ca, cb in tdroox.child_spans(row, "w:tc"):
                cell = row[ca:cb]
                for pa, pb in tdroox.child_spans(cell, "w:p"):
                    units.append(("cell", cell[pa:pb]))
    else:
        for pa, pb in tdroox.child_spans(xml, "w:p"):
            units.append(("", xml[pa:pb]))
    for cell, para in units:
        runs = tdroox.iter_runs(para)
        for sp in tdroox.merge_highlight_spans(runs):
            spans.append({
                "text": sp["text"],
                "run_range": [sp["run_range"][0], sp["run_range"][1]],
                "in_field": sp["in_field"],
                "in_field_result": sp["in_field_result"],
                "cell": cell,
            })
        for r in runs:
            if (not r.has_text) and r.highlight and r.fld is None and r.instr is None:
                hl_empty += 1
            if r.highlight and (r.fld is not None or r.instr is not None):
                hl_field += 1
            # CA-04②：含 br/tab 的高亮文本 run 不入跨度（跨度永不跨界）——
            # 计入 hl_break 保持会计学闭合（accounted=总数）
            if r.highlight and r.has_text and r.has_break \
                    and r.fld is None and r.instr is None:
                hl_break += 1
        # 域字符/instrText 自身高亮（H-01：空 w:t run 的主因=题注域体）
        if tdroox.para_mark_highlight(para):
            hl_mark += 1
    return {"spans": spans, "hl_empty_runs": hl_empty, "hl_para_marks": hl_mark,
            "hl_field_runs": hl_field, "hl_break_runs": hl_break}


# ---------------------------------------------------------------------------
# 建议键名映射（T-05/T-08：digest 候选专项建议；人工确认检查单 D-18 把关）
# ---------------------------------------------------------------------------

# 短语 → 建议槽声明（dict 形态与 rules.json 槽字段一致；occurrence 占位在
# 生成阶段按实测出现次数填 1/"all"）
_SUGGEST_TEXT = {
    u"（自动输出软件名称）": {"bind": "model_stats.software_name",
                              "note": u"双值句首期单键单值，页签①可录入覆盖（D-24①）"},
    u"（自动输出单元类型）": {"bind": "manual_fields.element_type",
                              "note": u"U-06：人工补录/规则默认值，不得标缺"},
    u"（节点数）": {"bind": "model_stats.node_count"},
    u"（单元数）": {"bind": "model_stats.element_count"},
    u"（载荷类型）": {"bind": "case.load_type",
                     "note": u"循环模板块内实例键（case.{i} 改写）"},
    u"（约束形式描述）": {"bind": "manual_fields.constraint_desc"},
    u"（加载形式描述）": {"bind": "manual_fields.load_case_desc",
                          "note": u"T-12②：字面为（加载形式描述）"},
    u"（计算方法）": {"bind": "manual_fields.calc_method"},
    u"（剩余系数数值=材料极限/最大应力）": {"bind": "case.results.reserve_factor",
                                                "note": u"C7/D-4 首期标缺"},
    u"（）": {"bind": "case.results.displacement_max",
              "note": u"T-05：空括号占位专项建议键名 displacement_max"},
    u"XXX": {"bind": "manual_fields.project_subject",
             "note": u"D-24②：未录入=保留原文+告警（missing_policy=keep_highlight）"},
}


# ---------------------------------------------------------------------------
# T0a 建议表外置（伴生覆盖内置）+ 归一化匹配；T0b 锚点质量 lint
# ---------------------------------------------------------------------------

def normalize_anchor_text(text):
    # type: (str) -> str
    """锚点字面归一化（T0a 匹配口径）：去首尾空白→全角括号折半角→
    去全部半角/全角括号与内部空白后比较（仅用于建议表匹配，槽 anchor
    match 声明仍保持源字面，引擎契约不变）。"""
    t = text.strip()
    t = t.replace(u"（", "(").replace(u"）", ")")
    t = re.sub(r"[\s()]+", "", t)
    return t


def _lint_anchor_text(text):
    # type: (str) -> Optional[str]
    """锚点字面质量检查（T0b lint，不阻断）：返回问题原因或 None。
    归一化后为空（如"（）"）/疑似占位泛词（XXX/NNN/待定/TODO）/归一化
    后长度<2 → 均出 WARNING 进 warns 通道（digest 报告+向导可见）。"""
    n = normalize_anchor_text(text)
    if n == u"":
        return u"归一化后为空（如空括号占位），引擎无法据其定位语义"
    up = n.upper()
    if (re.match(r"^X{3,}$", up) or re.match(r"^N{3,}$", up)
            or u"待定" in text or u"TODO" in up):
        return u"疑似占位泛词（XXX/NNN/待定/TODO 类）"
    if len(n) < 2:
        return u"归一化后长度<2，锚点过于含糊"
    return None


def _load_suggest_table(src_docx_path, suggest_path, warns):
    # type: (str, Optional[str], List[str]) -> Tuple[Dict[str, Any], Dict[str, Any]]
    """T0a：装载建议表（内置+伴生逐条合并，伴生优先）。
    伴生来源：显式 --suggest 路径，或缺省=源 docx 同目录同名
    XXX.suggest.jsonc 自动发现。JSONC 口径=整行 # 或 // 注释剥离
    （与 rules.jsonc/project_config 同源，不支持行尾注释）。
    伴生文件不存在（仅显式指定时告警）/解析失败/条目非法→内置表+WARNING，
    不阻断。返回 (merged_table, meta)；meta 含来源与伴生文件 sha1（写入
    manifest/report 溯源）。"""
    table = dict(_SUGGEST_TEXT)   # 浅拷贝：内置条目只读共享
    companion = suggest_path
    if companion is None:
        cand = os.path.splitext(src_docx_path)[0] + u".suggest.jsonc"
        if os.path.isfile(cand):
            companion = cand
    if companion is None:
        return table, {"source": "builtin", "companion": None, "sha1": None}
    if not os.path.isfile(companion):
        # 自动发现路径仅存在才装载（无告警噪音）；显式传参不存在必须告警
        if suggest_path is not None:
            warns.append(u"--suggest 指定的建议表文件不存在，改用内置表：%s"
                         % companion)
        return table, {"source": "builtin", "companion": None, "sha1": None}
    try:
        with io.open(companion, "r", encoding="utf-8-sig") as f:
            raw = f.read()
        clean = "\n".join(
            ln for ln in raw.splitlines()
            if not ln.lstrip().startswith(u"#") and not ln.lstrip().startswith(u"//"))
        data = json.loads(clean)
        if (not isinstance(data, dict)
                or not isinstance(data.get("suggest"), dict)):
            raise ValueError(u"顶层须为对象且含 \"suggest\" 对象（字面→{bind,note}）")
        n_merged = 0
        for key, entry in data["suggest"].items():
            if (not isinstance(key, str) or not isinstance(entry, dict)
                    or not isinstance(entry.get("bind"), str) or not entry["bind"]):
                warns.append(u"建议表伴生条目非法（缺 bind 或非对象），已跳过：%r"
                             % (key,))
                continue
            table[key] = {"bind": entry["bind"]}
            if entry.get("note"):
                table[key]["note"] = entry["note"]
            n_merged += 1
        meta = {"source": "companion", "companion": os.path.basename(companion),
                "sha1": sha1_bytes(raw.encode("utf-8")),
                "entries": len(table), "merged": n_merged}
        sys.stdout.write(u"[digest] 建议表：伴生 %s（合并 %d 条，总 %d 条，伴生覆盖内置）\n"
                         % (meta["companion"], n_merged, len(table)))
        sys.stdout.flush()
        return table, meta
    except Exception as e:
        warns.append(u"建议表伴生文件解析失败，改用内置表（%s）：%s"
                     % (os.path.basename(companion), e))
        return dict(_SUGGEST_TEXT), {"source": "builtin", "companion": None,
                                     "sha1": None}


def _suggest_lookup(table, text, warns):
    # type: (Dict[str, Any], str, List[str]) -> Optional[Dict[str, Any]]
    """建议表查找（T0a 匹配口径）：字面精确命中优先；否则归一化匹配，
    唯一命中即用（新命中，附 WARNING 供人工确认）；一字面归一化命中多条
    建议=取第一条+WARNING。未命中返回 None（槽声明不产出）。"""
    sug = table.get(text)
    if sug is not None:
        return sug
    n = normalize_anchor_text(text)
    if not n:
        return None
    hits = [k for k in table if normalize_anchor_text(k) == n]
    if not hits:
        return None
    if len(hits) == 1:
        warns.append(u"建议表归一化命中（T0a 非字面精确）：锚点 %r → 建议条目 %r"
                     u"（bind=%s，需人工确认）" % (text, hits[0], table[hits[0]]["bind"]))
        return table[hits[0]]
    warns.append(u"锚点 %r 归一化命中 %d 条建议（%s），取第一条 %r（bind=%s，"
                 u"需人工确认）" % (text, len(hits), u"/".join(hits[:4]),
                                    hits[0], table[hits[0]]["bind"]))
    return table[hits[0]]


def _table_facts(blk):
    # type: (Block) -> Dict[str, Any]
    """表格事实回读（T-09/C3-03：行列/gridSpan/表头高亮格/vMerge 行组/OLE）"""
    xml = blk.xml()
    grid = re.findall(r'<w:gridCol w:w="(\d+)"/>', xml)
    rows_out = []
    v_merge = {}
    header_hl = False
    ole = False
    for ri, (ra, rb) in enumerate(tdroox.child_spans(xml, "w:tr")):
        row = xml[ra:rb]
        cells = []
        for ci, (ca, cb) in enumerate(tdroox.child_spans(row, "w:tc")):
            cell = row[ca:cb]
            vm = re.search(r'<w:vMerge(?:\s+w:val="(\w+)")?\s*/>', cell)
            hl_cells = len(re.findall(r'w:highlight[^>]*w:val="yellow"', cell))
            if vm:
                v_merge["r%dc%d" % (ri, ci)] = vm.group(1) or "continue"
            if ri == 0 and hl_cells:
                header_hl = True
            if "<w:object" in cell:
                ole = True
            cells.append(tdroox.fragment_text(cell))
        rows_out.append(cells)
    grid_span = {}
    m = re.search(r"<w:tr[ >].*?</w:tr>", xml, re.S)
    if m:
        for ci, (ca, cb) in enumerate(tdroox.child_spans(m.group(0), "w:tc")):
            gs = re.search(r'<w:gridSpan w:val="(\d+)"/>', m.group(0)[ca:cb])
            if gs and gs.group(1) != "1":
                grid_span[str(ci)] = int(gs.group(1))
    return {
        "block": blk.id,
        "order": -1,   # 由调用方回填（blocks 序）
        "rows": len(rows_out),
        "cols": len(grid),
        "text": "".join("".join(r) for r in rows_out),
        "header_text": rows_out[0] if rows_out else [],
        "all_text": rows_out,
        "grid_span": grid_span,
        "header_cells_hl": header_hl,
        "v_merge": v_merge,
        "has_ole": ole,
    }


def _suggest_loop_chapter(blocks, tables):
    # type: (List[Block], List[Dict[str, Any]]) -> Tuple[Optional[Dict[str, Any]], List[str]]
    """循环章候选草稿启发式（advisory，人工确认 D-18）：
    - template_range：含"工况一"字面的章标题 → 到下一章标题前；
    - exclude_subsections：体为"（同上）/同上"的死子节；
    - dead_chapters：正文整体≈"同上"的废弃章；
    - literal_rewrite：从模板块提取重复工况名字面（章标题去编号/校核）。
    返回 (loop 声明或 None, warns)。"""
    warns = []
    tpl_head = None
    dead = []
    for blk in blocks:
        if blk.is_heading and u"工况一" in blk.heading_text:
            tpl_head = blk
            break
    if tpl_head is None:
        return None, warns
    # 范围终点：tpl_head 之后第一个章标题块
    to_head = None
    for blk in blocks:
        if blk.chapter > tpl_head.chapter and blk.is_heading:
            to_head = blk
            break
    if to_head is None:
        return None, warns
    excludes = []
    range_blocks = [b for b in blocks
                    if tpl_head.chapter <= b.chapter < to_head.chapter]
    i = 0
    while i < len(range_blocks):
        blk = range_blocks[i]
        if blk.is_subsection:
            body_until = []
            j = i + 1
            while j < len(range_blocks) and not range_blocks[j].is_subsection \
                    and not range_blocks[j].is_heading:
                body_until.append(range_blocks[j])
                j += 1
            joined = "".join(b.text.strip() for b in body_until)
            if joined.replace(u"（同上）", "").replace(u"同上", "").strip() == "" and body_until:
                excludes.append(blk.heading_text)
            i = j
        else:
            i += 1
    # dead 章判断：tpl 范围终点章（=to_before 标题所在章，即首个候选）起的
    # 章体≈"同上"（htsy 实测 dead=第 7 章=to_before_heading 所指章本身）
    for ch_no in range(to_head.chapter, max(b.chapter for b in blocks) + 1):
        ch_blocks = [b for b in blocks if b.chapter == ch_no]
        if not ch_blocks:
            continue
        joined = "".join(b.text.strip() for b in ch_blocks[1:] if not b.is_heading)
        if joined and joined.replace(u"同上", u"").strip() == u"":
            dead.append(ch_blocks[0].heading_text)
    # literal 字面：章标题去编号/空格/"校核" 后的工况名
    literal = re.sub(r"^[\d.\s]+", "", tpl_head.heading_text)
    literal = literal.replace(u"校核", u"").strip()
    loop = {
        "id": "case_check",
        "template_range": {
            "from_heading": tpl_head.heading_text,
            "to_before_heading": to_head.heading_text,
            "exclude_subsections": excludes,
        },
        "dead_chapters": dead,
        "output_form": "full_chapter_per_case",
        "literal_rewrite": [
            {"match": literal, "scope": "template_range",
             "rewrite_to": "case.{i}.name"},
        ],
        "instance_key": "dataset.cases",
        "instance_binding": "case.{i}",
        "renumber": {"heading": "literal:K_i=5+i",
                     "subsection": "literal:K_i.{1,2,3}",
                     "following_shift": True},
        "static_enum_policy": "keep_and_warn",
    }
    if literal != u"工况一":
        warns.append(u"循环章字面启发式提取=%r（预期 工况一，请人工确认）" % literal)
    return loop, warns


def _suggest_image_slots(blocks, tables, loop_literal):
    # type: (List[Block], List[Dict[str, Any]], str) -> List[Dict[str, Any]]
    """图片槽候选草稿（三形态，T-08/D-10）：
    形态1=占位段落文本（（网格划分图））；形态2=空段落坐标（题注"图N"前
    的纯空段——排除已含图/既存候选）；形态3=表格坐标（含 A）~D）子题注的
    结果图注表）。返回槽声明列表。"""
    slots = []   # type: List[Dict[str, Any]]
    keys = ["mesh", "boundary", "buckle", "disp", "stress_c", "stress_d"]
    # 形态 1
    for blk in blocks:
        if blk.kind != "para":
            continue
        if u"（网格划分图）" in blk.text:
            slots.append({
                "kind": "image", "bind": "image.mesh.path",
                "anchor": {"match": u"（网格划分图）"},
                "size": {"policy": "fit_section_width"},
                "missing_policy": "dash_and_warn",
            })
            break
    # 形态 3：含 A）/B）/C）/D）子题注的表
    sub_caps = [u"A）", u"B）", u"C）", u"D）"]
    for t in tables:
        joined = t.get("text", "")
        if all(c in joined for c in sub_caps) and t["has_ole"] is False:
            img_keys = ["buckle", "disp", "stress_c", "stress_d"]
            cells = ["r0c0", "r0c1", "r2c0", "r2c1"]
            cap_cells = ["r1c0", "r1c1", "r3c0", "r3c1"]
            # 锚点字面=表内实际子题注字面（"失稳模态"——表块文本直接命中；
            # 逐格坐标天然区分 C/D 同字面，T-08/D-10）
            for k, cell, cap_cell in zip(img_keys, cells, cap_cells):
                slots.append({
                    "kind": "image", "bind": "image.%s.path" % k,
                    "anchor": {"element": "table", "table_match": u"失稳模态",
                               "cell": cell},
                    "caption_cell": cap_cell,
                    "size": {"policy": "fit_cell"},
                    "missing_policy": "dash_and_warn",
                })
            break
    # 形态 2：题注段（文本以 图N 开头且无 SEQ 域缺失）之前的纯空段
    for i, blk in enumerate(blocks):
        if blk.kind != "para" or not blk.text.strip():
            continue
        if not re.match(r"^图\d+ ", blk.text.strip()):
            continue
        prev = blocks[i - 1] if i > 0 else None
        if prev is None or prev.kind != "para" or prev.text.strip():
            continue
        prev_xml = prev.xml()
        if ("<w:drawing" in prev_xml or "<w:pict" in prev_xml
                or "<w:object" in prev_xml or "w:highlight" in prev_xml):
            continue   # 已含图/占位非空段
        if u"图7" in blk.text.strip():
            slots.append({
                "kind": "image", "bind": "image.boundary.path",
                "anchor": {"element": "block_paragraph", "block": prev.id},
                "size": {"policy": "fit_section_width"},
                "missing_policy": "dash_and_warn",
                "_note": u"空段落坐标锚点（图7 模型边界及加载），digest 候选坐标人工确认",
            })
            break
    return slots


def _build_draft_rules(blocks, tables, sections, fields, book_marks,
                       ole_objects, anchor_records, literal_records, warns,
                       suggest_resolved=None):
    # type: (List[Block], List[Dict[str, Any]], List[Dict[str, Any]], List[Dict[str, Any]], List[Dict[str, Any]], List[Dict[str, Any]], List[Dict[str, Any]], List[Dict[str, Any]], List[str], Optional[Dict[str, Dict[str, Any]]]) -> Tuple[Dict[str, Any], Dict[int, str]]
    """规则层候选草稿（六层结构）+ 跨度处置归类表 {span_idx: category}。
    归类：slot（槽声明命中）/keep（keep_literals）/engine_seq_cache（S5-4）/
    engine_caption（R3-01 循环题注域后子跨度）。
    suggest_resolved：digest 预解析的 字面→建议条目 表（T0a，含伴生覆盖与
    归一化命中结果；None=内置 _SUGGEST_TEXT 兼容）。"""
    categories = {}   # type: Dict[int, str]
    slots = []        # type: List[Dict[str, Any]]
    declared_texts = []   # type: List[str]
    if suggest_resolved is None:
        suggest_resolved = _SUGGEST_TEXT
    loop, loop_warns = _suggest_loop_chapter(blocks, tables)
    warns.extend(loop_warns)
    loop_literal = u""
    if loop:
        loop_literal = loop["literal_rewrite"][0]["match"]
    range_block_ids = set()
    if loop:
        # 解析模板范围块（供图8/表8 题注域后子跨度归类 engine_caption）
        from_ids = [b.id for b in blocks if b.is_heading
                    and b.heading_text == loop["template_range"]["from_heading"]]
        to_ids = [b.id for b in blocks if b.is_heading
                  and b.heading_text == loop["template_range"]["to_before_heading"]]
        if from_ids and to_ids:
            fi = next(i for i, b in enumerate(blocks) if b.id == from_ids[0])
            ti = next(i for i, b in enumerate(blocks) if b.id == to_ids[0])
            range_block_ids = set(b.id for b in blocks[fi:ti])
    # 1) 文本槽候选（高亮跨度建议命中）
    phrase_occ = {}   # type: Dict[str, int]
    for rec in anchor_records:
        text = rec["text"]
        if rec.get("in_field_result"):
            categories[rec["_i"]] = "engine_seq_cache"
            continue
        if rec["block"] in range_block_ids and rec.get("has_seq_field"):
            # 循环题注段全部高亮子跨度：实例化题注语义隐式承接（R3-01 引擎
            # 职责——域后子跨度工况名改写+整段清高亮，域结构保留；图8/表8 同构）
            categories[rec["_i"]] = "engine_caption"
            continue
        sug = suggest_resolved.get(text)
        if sug is None:
            continue
        if text not in declared_texts:
            slot = {"kind": "text", "anchor": {"match": text,
                                               "occurrence": sug.get("occurrence", 1)},
                    "bind": sug["bind"]}
            if text == u"XXX":
                slot["missing_policy"] = "keep_highlight"
            if sug.get("note"):
                slot["_note"] = sug["note"]
            slots.append(slot)
            declared_texts.append(text)
        categories[rec["_i"]] = "slot"
    # 1') 非高亮字面槽（XXX 等字面锚点——不在高亮跨度池，D-24②）
    for rec in literal_records:
        if "suggest_bind" not in rec or rec.get("category") is None:
            continue
        sug = suggest_resolved.get(rec["text"])
        if sug is None or rec["text"] in declared_texts:
            continue
        slots.append({
            "kind": "text",
            "anchor": {"match": rec["text"],
                       "occurrence": "all" if rec.get("phrase_total", 1) >= 2 else 1},
            "bind": sug["bind"],
        })
        if rec["text"] == u"XXX":
            slots[-1]["missing_policy"] = "keep_highlight"
            slots[-1]["anchor"]["occurrence"] = "all"
            slots[-1]["_note"] = sug.get("note")
        declared_texts.append(rec["text"])
    # 同字面多处的 occurrence="all"（D-24①：软件名称双处单键；XXX 双处）
    for slot in slots:
        if slot["kind"] != "text":
            continue
        text = slot["anchor"]["match"]
        n = sum(1 for rec in anchor_records
                if rec["text"] == text and not rec.get("in_field_result"))
        if slot["anchor"]["match"] == u"XXX" or text == u"（自动输出软件名称）":
            if n >= 2:
                slot["anchor"]["occurrence"] = "all"
    # 2) 表格槽（表7 计算载荷 / 表8 应力及强度剩余系数；C3-03 行组声明）
    for t in tables:
        header_join = "".join(t["header_text"])
        if u"端面载荷" in header_join:
            slots.append({
                "kind": "table",
                "anchor": {"match": u"计算载荷", "element": "table",
                           "expect": "%drows:%dcols(grid)" % (t["rows"], t["cols"])},
                "header_row": 0,
                "header_policy": "keep_clear_highlight",
                "data_rows": [1],
                "grid_span": t["grid_span"],
                "columns": [
                    {"col": 0, "bind": "case.name"},
                    {"col": 1, "bind": "case.loads.position"},
                    {"col": 2, "bind": "case.loads.end_load"},
                    {"col": 3, "bind": "case.loads.pressure"},
                    {"col": 4, "bind": "case.loads.temperature"},
                ],
                "grow": "copy_row_template",
                "shrink": "delete_extra_rows",
                "_note": u"表7 计算载荷（T-09 实测列集；端面载荷 gridSpan=2 跨列映射）",
            })
            for rec in anchor_records:
                if rec["block"] == t["block"]:
                    categories[rec["_i"]] = "slot"
        elif t["has_ole"] and t.get("v_merge"):
            # 表8：行组模板（C3-03/H-02：r1 restart/r2 continue 材料格竖合并）
            restart_rows = sorted(set(int(k[1:].split("c")[0])
                                      for k, v in t["v_merge"].items() if v == "restart"))
            merge_col = sorted(set(int(k.split("c")[1])
                                   for k, v in t["v_merge"].items()))
            row_templates = [0, restart_rows[0] if restart_rows else 1, restart_rows[0] + 1 if restart_rows else 2]
            slots.append({
                "kind": "table",
                "anchor": {"match": u"应力及强度剩余系数", "element": "table",
                           "expect": "%drows:%dcols" % (t["rows"], t["cols"])},
                "header_row": 0,
                "header_policy": "keep_clear_highlight",
                "row_templates": row_templates,
                "row_group": {"merge_col": merge_col[0] if merge_col else 1,
                              "restart_row": restart_rows[0] if restart_rows else 1},
                "columns": [
                    {"col": 0, "bind": "keep_literal"},
                    {"col": 1, "bind": "materials.name"},
                    {"col": 2, "bind": "materials.max_stress"},
                    {"col": 3, "bind": "materials.reserve_factor"},
                ],
                "grow": "copy_row_template",
                "shrink": "delete_extra_rows",
                "_note": u"表8 行组模板+vMerge 语义（C3-03）：成组复制/成组删除；"
                         u"materials.* 全局共享键（H-07 白名单）；c3 表头内联 OLE 不触及（T-22）",
            })
            for rec in anchor_records:
                if rec["block"] == t["block"]:
                    categories[rec["_i"]] = "slot"
    # 3) 图片槽（三形态）
    img_slots = _suggest_image_slots(blocks, tables, loop_literal)
    slots.extend(img_slots)
    img_table_blocks = set()
    for s in img_slots:
        a = s.get("anchor") or {}
        if a.get("match"):
            for rec in anchor_records:
                if rec["text"] == a["match"]:
                    categories[rec["_i"]] = "slot"
        if a.get("table_match"):
            for t in tables:
                if a["table_match"] in (t.get("text") or ""):
                    img_table_blocks.add(t["block"])
    for rec in anchor_records:
        if rec["block"] in img_table_blocks and rec["_i"] not in categories:
            categories[rec["_i"]] = "slot"   # 子题注格跨度=图片槽 caption_cell 承接
    # 4) 条件措辞槽（C3-07 执行序=句内槽先于整句槽；CC-01：按扫描结果生成——
    # 仅当锚点前缀在块文本中唯一命中时才产出候选，任意不含该句式的模板不再
    # 生成"引擎必拒载"的死槽（提炼产物可独立运行，FR-12/E5）；命中多块=登记
    # warn 交人工确认，绝不静默丢弃定位信息）
    cond_prefix = u"通过有限元计算分析"
    cond_pool = [b for b in blocks if b.text.strip().startswith(cond_prefix)]
    if len(cond_pool) == 1:
        slots.append({
            "kind": "conditional_text",
            "anchor": {"match_prefix": cond_prefix},
            "bind": "conclusion.verdict",
            "branches": [
                {"when": "reserve_factor_min >= 1.0", "use": "conclusion.pass"},
                {"when": "reserve_factor_min < 1.0", "use": "conclusion.fail"},
            ],
            "default_branch": {"use": "missing_text"},
            "missing_text": u"（计算结果待补充）",
            "_note": u"C7/D-4：判定键 null→整句替换为槽级缺省文案；分支文案支持"
                     u" {manual_fields.project_subject} 全限定键引用（H-14/C12-Q-01，"
                     u"裸键引用=校验期条目级错误）；句内 XXX 槽先于本槽执行（C3-07）",
        })
    elif len(cond_pool) > 1:
        warns.append(u"结论句前缀 %r 命中 %d 个块（须唯一），未生成条件措辞槽候选"
                     u"（CC-01：按扫描结果生成，需人工确认）" % (cond_prefix, len(cond_pool)))
    # 5) OLE 处置声明（C8 首期仅 preserve）
    for o in ole_objects:
        slots.append({
            "kind": "ole",
            "anchor": {"ole_prog_id": o["prog_id"], "occurrence": o.get("occurrence", 1)},
            "policy": "preserve",
            "_note": u"实测位置=表8 表头 r0c3 剩余系数格内联（T-12③）；"
                     u"清高亮/表头声明不触及对象 run（T-22 互斥）",
        })
    # 6) keep_literals：未归类的高亮文本跨度（原样保留+清高亮）
    keep_literals = []
    for rec in anchor_records:
        if rec["_i"] in categories:
            continue
        keep_literals.append({
            "anchor": {"match": rec["text"], "block": rec["block"]},
            "reason": u"题注域前/域外标签字面（hl_%03d），原样保留+清高亮" % rec["_i"],
        })
        categories[rec["_i"]] = "keep"
    # L1 数据字典（03 §4.3 对照表键集）
    data_dictionary = {
        "model_stats.software_name": "string",
        "model_stats.node_count": "int",
        "model_stats.element_count": "int",
        "manual_fields.element_type": "string",
        "manual_fields.constraint_desc": "string",
        "manual_fields.load_case_desc": "string",
        "manual_fields.load_type": "string",
        "manual_fields.calc_method": "string",
        "manual_fields.project_subject": "string",
        "meta.title": "string",
        "meta.author": "string",
        "meta.company": "string",
        "meta.date": "string",
        "meta.calc_file": "string",
        "case.seq": "int",
        "case.name": "string",
        "case.load_type": "string",
        "case.loads.*": "object",
        "case.loads.position": "string",
        "case.loads.end_load": "string",
        "case.loads.pressure": "string",
        "case.loads.temperature": "string",
        "case.results.*": "object",
        "case.results.stress_peak": "number",
        "case.results.displacement_max": "number",
        "case.results.buckling_mode": "string",
        "case.results.reserve_factor": "number",
        "materials.name": "string",
        "materials.allowable_stress": "number",
        "materials.max_stress": "number",
        "materials.reserve_factor": "number",
        "image.mesh.path": "string",
        "image.boundary.path": "string",
        "image.buckle.path": "string",
        "image.disp.path": "string",
        "image.stress_c.path": "string",
        "image.stress_d.path": "string",
        "conclusion.verdict": "number",
    }
    rules = {
        "meta": {
            "format_version": tdrrules.RULES_FORMAT_VERSION,
            "template_id": "",
            "display_name": "",
            "highlight_contract": "all-must-declared",
            "_scan_scope": u"word/document.xml body 主文档流（C3-08/H-08）",
        },
        "data_dictionary": data_dictionary,
        "slots": slots,
        "loop_chapters": [loop] if loop else [],
        "keep_literals": keep_literals,
        "fidelity": {
            "baseline": "assets_as_extracted",
            "untouched_parts": "byte_identical",
            "allowed_transforms": [
                "placeholder_replace", "highlight_clear", "table_grow_shrink",
                "loop_instantiate", "image_insert", "heading_renumber",
                "bookmark_rewire", "ole_preserve", "toc_dirty_mark",
                "conditional_render",
            ],
            "section_header_footer": "keep_references",
        },
        "exception_policy": {
            "missing_data": "dash_and_warn",
            "missing_text": "-",
            "broken_template": "degrade_and_warn",
            "oversize_image": "fit_and_warn",
            "_note": u"整句措辞槽缺省=整句替换槽级 missing_text（如（计算结果待补充））；"
                     u"标缺统一半角 -（与模板全角 — 专义区分，D-05）",
        },
    }
    return rules, categories


# ---------------------------------------------------------------------------
# 主消化流程
# ---------------------------------------------------------------------------

def digest(src_docx_path, out_dir, suggest_path=None):
    # type: (str, str, Optional[str]) -> Dict[str, Any]
    """主入口（FR-11/12）：返回消化报告 dict；容器落盘 <out_dir>/<template_id>.rtdoc；
    报告落盘 <out_dir>/<template_id>_digest_report.json/.txt。
    第一步=⓪ E6 前置校验（失败抛 DigestBlocked，不产出任何文件）。
    REF-01：占用探测为读试开口径——可读但有写锁/只读属性时照常消化，
    消化报告 warnings 追加编辑中提示。
    suggest_path：T0a 建议表伴生文件显式路径（None=同名 .suggest.jsonc
    自动发现，无伴生=内置表；装载来源与哈希写入 manifest/report 溯源）。"""
    z, src_write_locked = verify_input(src_docx_path)   # ⓪ E6
    try:
        # 输入上限体检（S5-3/CA-05/CA-08：零解压中央目录预检——parts/单件/
        # 总体积/解压比率 max_ratio 首次真实生效——先于任何解压发生，zip 炸弹
        # 不再先撑爆内存才被拒绝）
        tdroox.zip_preflight_z(z, INPUT_LIMITS)
        # 限量流式读取（CC-14：坏 CRC 等读取失败→可读 DigestBlocked 文案，
        # 不再裸 traceback；rc 与零产出硬性面不变）
        try:
            pkg = tdroox.read_package_z_limited(z, INPUT_LIMITS)
        except tdroox.PackageLimitError as e:
            raise DigestBlocked(u"%s" % e)
        except Exception as e:
            raise DigestBlocked(u"文件损坏或非 docx 格式（部件读取失败）：%s" % e)
    finally:
        z.close()

    warns = []   # type: List[str]
    if src_write_locked:
        # REF-01（C15）：共享可读但存在写锁/只读属性——不阻断，提示按磁盘
        # 最近保存版本消化（作为首条 warn 进入消化报告 json/txt 正文）
        warns.append(u"源文件可能正在 Word 中编辑，本次按最近一次保存到磁盘的"
                     u"版本消化（不含未保存修改），建议先保存并关闭以确保基线正确")
    doc = pkg["word/document.xml"].decode("utf-8")
    # DX-08：digest 阶段进度提示（此前全程静默，大 docx 长时间无响应感）
    sys.stdout.write(u"[digest] 消化中：解析正文/切块/高亮归并/规则草稿生成…\n")
    sys.stdout.flush()
    body_m = re.search(r"<w:body>(.*)</w:body>", doc, re.S)
    if body_m is None:
        raise DigestBlocked(u"document.xml 无 body（结构性损坏，E5 提炼期阻断）")
    body = body_m.group(1)

    # ② 切块 + 章节二级索引
    blocks, merge_warns = split_blocks(body)
    warns.extend(merge_warns)

    # 块序/表格事实
    tables = []   # type: List[Dict[str, Any]]
    for order, blk in enumerate(blocks):
        if blk.kind == "table":
            tf = _table_facts(blk)
            tf["order"] = order
            tables.append(tf)

    # sections（版心参数，3 节结构 FR-33）
    sections = []
    for sm in re.finditer(r"<w:sectPr[^>]*>.*?</w:sectPr>", doc, re.S):
        s = sm.group(0)
        pgsz = re.search(r"<w:pgSz[^>]*/>", s)
        pgmar = re.search(r"<w:pgMar[^>]*/>", s)
        wv = re.search(r'w:w="(\d+)"', pgsz.group(0)) if pgsz else None
        hv = re.search(r'w:h="(\d+)"', pgsz.group(0)) if pgsz else None
        mar = [int(x) for x in re.findall(r'w:(?:top|right|bottom|left)="(\d+)"', pgmar.group(0))] if pgmar else []
        sections.append({
            "sect_width": int(wv.group(1)) if wv else 0,
            "sect_height": int(hv.group(1)) if hv else 0,
            "margins": mar,
        })

    # ⑥ 域清单/书签清单/OLE（R5/H-10/H-11）
    fld_entries = []   # type: List[Dict[str, Any]]
    for order, blk in enumerate(blocks):
        xml = blk.xml()
        for im in re.finditer(r'<w:instrText[^>]*>([^<]*)</w:instrText>', xml):
            instr = im.group(1).strip()
            ftype = ("TOC" if instr.startswith("TOC")
                     else "PAGEREF" if instr.startswith("PAGEREF")
                     else "SEQ" if instr.startswith("SEQ")
                     else "REF" if instr.startswith("REF")
                     else "PAGE" if instr.startswith("PAGE")
                     else "NUMPAGES" if instr.startswith("NUMPAGES") else instr.split(" ")[0])
            fld_entries.append({"type": ftype, "instr": instr,
                                "block": blk.id, "order": order})
    # PAGEREF 死条目登记（H-10：目标书签缺失=已知残留，刷新自愈）
    bm_names = set()
    book_marks = []   # type: List[Dict[str, Any]]
    for order, blk in enumerate(blocks):
        for bmm in re.finditer(r'<w:bookmarkStart w:id="(\d+)" w:name="([^"]+)"', blk.xml()):
            book_marks.append({"id": bmm.group(1), "name": bmm.group(2),
                               "block": blk.id, "order": order,
                               "cursor_bookmark": bmm.group(2) == "_GoBack"})
            bm_names.add(bmm.group(2))
    dead_pageref = []
    for f in fld_entries:
        if f["type"] == "PAGEREF":
            target = f["instr"].split()[1] if len(f["instr"].split()) > 1 else ""
            if target and target not in bm_names:
                dead_pageref.append({"instr": f["instr"], "block": f["block"],
                                     "note": u"PAGEREF 目标书签不存在（TOC 缓存死条目，"
                                             u"F9 刷新自愈——已知残留，非引擎缺陷，H-10/C3-08）"})
    ole_objects = []
    ole_occ = 0
    for order, blk in enumerate(blocks):
        for om in re.finditer(r'ProgID="([^"]+)"', blk.xml()):
            ole_occ += 1
            ole_objects.append({"prog_id": om.group(1), "block": blk.id,
                                "occurrence": ole_occ})

    # ③ 跨 run 高亮归并 + ③' 域存在性标注（题注段 has_seq_field 供归类）
    anchor_records = []   # type: List[Dict[str, Any]]
    hl_text_runs = 0
    hl_empty_runs = 0
    hl_field_runs = 0     # 域字符/instrText 自身高亮 run（H-01：题注域体高亮）
    hl_para_marks = 0
    hl_break_runs = 0     # CA-04②：含 br/tab 的高亮文本 run（不入跨度）
    for order, blk in enumerate(blocks):
        stat = scan_block_spans(blk)
        hl_empty_runs += stat["hl_empty_runs"]
        hl_para_marks += stat["hl_para_marks"]
        hl_field_runs += stat["hl_field_runs"]
        hl_break_runs += stat.get("hl_break_runs", 0)
        blk_has_seq = "SEQ" in blk.xml()
        for sp in stat["spans"]:
            hl_text_runs += (sp["run_range"][1] - sp["run_range"][0] + 1)
            anchor_records.append({
                "_i": len(anchor_records),
                "block": blk.id, "text": sp["text"], "run_range": sp["run_range"],
                "in_field": sp["in_field"], "in_field_result": sp["in_field_result"],
                "cell": sp["cell"], "has_seq_field": blk_has_seq,
                "chapter": blk.chapter, "is_highlight_span": True,
            })
    # occurrence 编号（同字面按文档序 1 起）
    phrase_count = {}   # type: Dict[str, int]
    for rec in anchor_records:
        t = rec["text"]
        phrase_count[t] = phrase_count.get(t, 0) + 1
        rec["occurrence"] = phrase_count[t]
        rec["id"] = "hl_%03d" % rec["_i"]
    unique_phrases = len(phrase_count)
    # 高亮会计学（128 = 文本 run + 域字符 run + 空 w:t run + 段落标记；03 §0；
    # CA-04②：br/tab 高亮文本 run 单列，保证 accounted=总数恒成立）
    hl_total = sum(len(re.findall(r'w:highlight[^>]*w:val="yellow"', b.xml()))
                   for b in blocks)
    accounted = (hl_text_runs + hl_empty_runs + hl_field_runs + hl_para_marks
                 + hl_break_runs)

    # 非高亮字面锚点（XXX 等槽匹配所需的字面索引；来源=处置字面报告检测）
    literal_records = _detect_literals(blocks, warns)

    # T0a 建议表装载（伴生覆盖内置）+ 唯一字面建议预解析（归一化命中
    # WARNING 每字面只记一次）；T0b 锚点质量 lint（WARNING 不阻断）
    suggest_table, suggest_meta = _load_suggest_table(
        src_docx_path, suggest_path, warns)
    suggest_resolved = {}   # type: Dict[str, Dict[str, Any]]
    lint_seen = set()
    for t in list(phrase_count.keys()) + [r["text"] for r in literal_records
                                          if str(r.get("id", "")).startswith("lit_")]:
        if t not in suggest_resolved:
            suggest_resolved[t] = _suggest_lookup(suggest_table, t, warns)
        reason = _lint_anchor_text(t)
        if reason and t not in lint_seen:
            lint_seen.add(t)
            warns.append(u"锚点字面质量提示（T0b lint，不阻断）：%r %s"
                         u"——建议模板作者修正措辞后重新消化" % (t, reason))

    # ⑦ 防御检测：B6 跨章 REF / C-11 循环块 w:pict / S5-1 特性清单
    loop, _lw = _suggest_loop_chapter(blocks, tables)
    range_block_ids = set()
    if loop:
        fids = [b.id for b in blocks if b.heading_text == loop["template_range"]["from_heading"]]
        tids = [b.id for b in blocks if b.heading_text == loop["template_range"]["to_before_heading"]]
        if fids and tids:
            fi = next(i for i, b in enumerate(blocks) if b.id == fids[0])
            ti = next(i for i, b in enumerate(blocks) if b.id == tids[0])
            range_block_ids = set(b.id for b in blocks[fi:ti])
    ref_errors = []
    for f in fld_entries:
        if f["type"] != "REF":
            continue
        target = f["instr"].split()[1] if len(f["instr"].split()) > 1 else ""
        tgt = next((b for b in book_marks if b["name"] == target), None)
        if tgt is None:
            continue
        src_blk = next((b for b in blocks if b.id == f["block"]), None)
        tgt_blk = next((b for b in blocks if b.id == tgt["block"]), None)
        if src_blk and tgt_blk and src_blk.chapter != tgt_blk.chapter \
                and tgt["block"] in range_block_ids:
            ref_errors.append({"instr": f["instr"], "block": f["block"],
                               "reason": u"REF 跨章且目标在循环模板块内（B6：实例化后"
                                         u"章外 REF 悬空风险，需人工确认）"})
    for bid in sorted(range_block_ids):
        b = next((x for x in blocks if x.id == bid), None)
        if b is not None and "<w:pict" in b.xml():
            warns.append(u"循环模板块 %s 含 w:pict（C-11：图片参与实例化，需人工确认）" % bid)
    feature_warns = _feature_scan(doc, blocks)
    warns.extend(feature_warns)

    # 页眉页脚/脚注/尾注高亮扫描（H-08/C3-04：warn 项登记；CC-20：补 endnotes.xml）
    hf_warns = []
    for n in sorted(pkg.keys()):
        if re.match(r"word/(header|footer)\d+\.xml$", n) \
                or n in ("word/footnotes.xml", "word/endnotes.xml"):
            x = pkg[n].decode("utf-8")
            hln = len(re.findall(r'w:highlight[^>]*w:val="yellow"', x))
            if hln:
                hf_warns.append(u"%s 含 %d 处高亮（超出 body 主文档流扫描范围——"
                                u"占位放在页眉页脚不会被槽机制消费，需人工处置）" % (n, hln))
            txt = tdroox.fragment_text(x)
            for lit in (u"共  35  页", u"XXX", u"0-0JW"):
                if lit in txt:
                    hf_warns.append(u"%s 含残留字面 %r（C3-04/U-07：首期引擎不改写页眉页脚，"
                                    u"登记+模板修订建议提交提供方）" % (n, lit))
    warns.extend(hf_warns)

    # ⑧ 规则层候选草稿
    draft, categories = _build_draft_rules(blocks, tables, sections, fld_entries,
                                           book_marks, ole_objects, anchor_records,
                                           literal_records, warns,
                                           suggest_resolved)
    template_id = re.sub(r"[^0-9A-Za-z\-_]+", "-", os.path.splitext(
        os.path.basename(src_docx_path))[0]).strip("-").lower() or "template"
    draft["meta"]["template_id"] = template_id
    draft["meta"]["display_name"] = os.path.splitext(os.path.basename(src_docx_path))[0]

    # 建议键名命中率/类型判别口径（FR-11 分母=唯一短语，C3-01；T0a：
    # 分子按字面精确命中口径统计——归一化命中不进 suggest_hits，仅新增
    # suggest 字段，保证无伴生时统计与改前一致）
    suggest_hits = sum(1 for t in phrase_count
                       if t in suggest_table or t in (u"图", u"表"))

    # index.json（锚点/表格/节/域/书签/字面）
    anchors_out = []
    for rec in anchor_records:
        anchors_out.append({
            "id": rec["id"], "block": rec["block"], "text": rec["text"],
            "run_range": rec["run_range"], "occurrence": rec["occurrence"],
            "phrase_total": phrase_count[rec["text"]],
            "in_field": rec["in_field"], "in_field_result": rec["in_field_result"],
            "cell": rec["cell"], "has_seq_field": rec["has_seq_field"],
            "chapter": rec["chapter"], "is_highlight_span": True,
            "managed": categories.get(rec["_i"], "undeclared"),
            "suggest_bind": (suggest_resolved.get(rec["text"], {}) or {}).get("bind"),
        })
    for rec in literal_records:
        if "id" not in rec:
            continue   # 处置报告条目（非锚点索引项），已在 literal_report 输出
        anchors_out.append({
            "id": rec["id"], "block": rec["block"], "text": rec["text"],
            "run_range": [], "occurrence": rec["occurrence"],
            "phrase_total": rec["phrase_total"], "in_field": False,
            "in_field_result": False, "cell": "", "has_seq_field": False,
            "chapter": rec["chapter"], "is_highlight_span": False,
            "managed": "literal_anchor", "suggest_bind": rec.get("suggest_bind"),
        })
    blocks_out = [{
        "id": b.id, "kind": b.kind, "style": b.style,
        "is_heading": b.is_heading, "is_subsection": b.is_subsection,
        "heading": b.heading_text or None, "chapter": b.chapter,
        "text": b.text,
    } for b in blocks]
    index = {
        "blocks": blocks_out,
        "anchors": anchors_out,
        "tables": [{k: v for k, v in t.items() if k != "all_text"} for t in tables],
        "sections": sections,
        "fields": {"fld": fld_entries, "bookmarks": book_marks,
                   "ole_objects": ole_objects, "dead_pageref": dead_pageref,
                   "ref_cross_chapter": ref_errors},
    }

    # 规则草稿三层校验自检（digest 自检：草稿必须可过校验，否则报 warn）
    errors = tdrrules.validate_rules(draft, index)
    for e in errors:
        warns.append(u"规则草稿校验：%s" % str(e))

    # manifest（30 部件去向对账表，T-01/C-01/D-21）
    parts_table = []
    for n in sorted(pkg.keys()):
        entry = {"source_part": n}
        if n == "word/document.xml":
            entry["store"] = "assets/content/blocks"
            entry["disposition"] = "content_blocks"
        elif n in ("[Content_Types].xml",):
            entry["store"] = "manifest:content_types"
            entry["disposition"] = "engine_rebuild"
        elif n == "_rels/.rels":
            entry["store"] = "manifest:package_rels"
            entry["disposition"] = "engine_rebuild"
        else:
            store = asset_store_name(n)
            if store is None:
                entry["store"] = ""
                entry["disposition"] = "explicit_dropped"
                entry["reason"] = u"未匹配资产映射（人工确认）"
            else:
                entry["store"] = store
                entry["disposition"] = "asset"
        entry["sha1"] = sha1_bytes(pkg[n])
        parts_table.append(entry)
    dispositioned = sum(1 for p in parts_table if p.get("disposition"))
    if dispositioned != len(parts_table):
        warns.append(u"manifest 对账：%d/%d 部件有去向（存在未登记部件，消化不完整）"
                     % (dispositioned, len(parts_table)))

    template_id_final = draft["meta"]["template_id"]
    with open(src_docx_path, "rb") as _sf:   # CA-14：句柄显式关闭（with 形态）
        _src_bytes = _sf.read()
    manifest = {
        "format_version": tdrrules.FORMAT_VERSION,
        "rules_format_version": tdrrules.RULES_FORMAT_VERSION,
        "template_id": template_id_final,
        "display_name": draft["meta"]["display_name"],
        "created": _now(),
        "source_docx": {"name": os.path.basename(src_docx_path),
                        "sha1": sha1_bytes(_src_bytes),
                        "parts": len(pkg)},
        "parts": parts_table,
        "blocks": {"count": len(blocks), "index": "assets/content/index.json"},
        "content_types": pkg["[Content_Types].xml"].decode("utf-8"),
        "package_rels": pkg["_rels/.rels"].decode("utf-8"),
        "document_xml_prefix": doc[:body_m.start()],
        "document_xml_suffix": doc[body_m.end():],
        "_wrapper_note": u"document.xml 壳（根标签命名空间+文尾）——引擎按块序重组 body 后"
                         u"经 M5 重建 document.xml（命名空间补齐基底）",
        "input_limits": dict(INPUT_LIMITS),
        "suggest_table": dict(suggest_meta),
        "_suggest_table_note": u"T0a 溯源：digest 所用建议键名表来源与伴生文件哈希"
                               u"（source=builtin/companion；伴生逐条覆盖内置），"
                               u"仅溯源，运行时不消费",
        "_source_sha1_note": u"仅溯源与重消化比对（R11），运行时不校验源 docx",
    }
    # CC-09：manifest 自身完整性锚——对"除锚外字段的规范化 JSON（sort_keys
    # 紧凑形态）"计算 SHA1 存入 manifest_sha1；引擎 M1 重算比对（篡改
    # content_types/上限等字段=完整性失败拒载）。旧格式容器无锚=引擎向后兼容跳过。
    manifest["manifest_sha1"] = sha1_bytes(json.dumps(
        dict((k, v) for k, v in manifest.items() if k != "manifest_sha1"),
        ensure_ascii=False, sort_keys=True).encode("utf-8"))

    # 容器落盘（zip：manifest+rules+assets+content blocks）
    container_path = os.path.join(out_dir, template_id_final + ".rtdoc")
    store = {}   # type: Dict[str, bytes]
    store["manifest.json"] = json.dumps(manifest, ensure_ascii=False,
                                        indent=1, sort_keys=True).encode("utf-8")
    store["rules/rules.json"] = _dump_rules_json(draft)
    for n, data in pkg.items():
        if n == "word/document.xml":
            continue
        st = asset_store_name(n)
        if st:
            store[st] = data
    for bi, blk in enumerate(blocks):
        store["assets/content/blk_%03d.xml" % bi] = blk.xml().encode("utf-8")
    store["assets/content/index.json"] = json.dumps(
        index, ensure_ascii=False, indent=1, sort_keys=True).encode("utf-8")
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)
    # DX-08：写盘阶段提示
    sys.stdout.write(u"[digest] 消化中：写容器与报告…\n")
    sys.stdout.flush()
    tdroox.write_package(store, container_path)

    # 消化报告（FR-11：候选清单+统计+字面报告+warn 汇总；RC-10：口径版本+自说明头）
    report = {
        "template_id": template_id_final,
        "container": os.path.abspath(container_path),
        "format_version": tdrrules.RULES_FORMAT_VERSION,
        "_note": u"消化报告（digest 自动生成，不应手改）：candidates=候选占位清单"
                 u"（人工确认），literal_report=非高亮需处置字面报告，"
                 u"warnings=特性与残留登记（warn 明细在 txt 尾节）",
        "stats": {
            "source_parts": len(pkg),
            "parts_dispositioned": dispositioned,
            "blocks": len(blocks),
            "chapters": max((b.chapter for b in blocks), default=0),
            "tables": len(tables),
            "sections": len(sections),
            "fields_fldchar_begin": len(re.findall(
                r'<w:fldChar w:fldCharType="begin"', doc)),
            "fields_instrtext": len(fld_entries),
            "bookmarks": len(book_marks),
            "cursor_bookmarks": len([b for b in book_marks if b["cursor_bookmark"]]),
            "ole_objects": len(ole_objects),
            "highlights_total_in_blocks": hl_total,
            "highlight_accounting": {
                "text_runs_in_spans": hl_text_runs,
                "field_char_runs": hl_field_runs,
                "empty_wt_runs": hl_empty_runs,
                "para_marks": hl_para_marks,
                "break_excluded_runs": hl_break_runs,
                "accounted": accounted,
                "formula": u"128 = 91 含文本 run（含 2 纯空白）+ 16 域字符 run 高亮"
                           u" + 1 空 w:t run + 20 段落标记（审计口径 89+19+20 在"
                           u"『空文本含纯空白』归并下与本文账目等价——H-01；"
                           u"CA-04②：跨度内 br/tab 高亮文本 run 单列不入账上式）",
            },
            "highlight_spans": len(anchor_records),
            "unique_phrases": unique_phrases,
            "span_count_note": u"C3-01 口径：33 跨度/29 唯一短语为评审预估，"
                               u"以本首跑实测回填为准（S5 digest 首跑实测）",
            "suggest_hits": suggest_hits,
            "suggest_rate": round(suggest_hits / float(unique_phrases), 4) if unique_phrases else 0.0,
            "candidates_for_confirm": sum(
                1 for a in anchors_out if a["is_highlight_span"]
                and not a["in_field_result"] and a["managed"] not in
                ("engine_caption", "keep") and a["suggest_bind"] is None),
            "engine_managed": sum(1 for a in anchors_out
                                  if a.get("managed") in ("engine_seq_cache", "engine_caption")),
        },
        "candidates": [
            {k: a[k] for k in ("id", "block", "text", "occurrence", "phrase_total",
                               "in_field", "in_field_result", "managed", "suggest_bind",
                               "chapter")}
            for a in anchors_out if a["is_highlight_span"]
        ],
        "literal_report": literal_records,
        "fields_dead_pageref": dead_pageref,
        "ref_cross_chapter": ref_errors,
        "suggest_table": dict(suggest_meta),
        "warnings": warns,
    }
    report_path = os.path.join(out_dir, template_id_final + "_digest_report.json")
    with io.open(report_path, "w", encoding="utf-8") as f:
        f.write(json.dumps(report, ensure_ascii=False, indent=1, sort_keys=True))
    with io.open(os.path.join(out_dir, template_id_final + "_digest_report.txt"),
                 "w", encoding="utf-8") as f:
        f.write(_render_report_text(report))
    return report


def _detect_literals(blocks, warns):
    # type: (List[Block], List[str]) -> List[Dict[str, Any]]
    """非高亮字面锚点索引（槽匹配所需）+ 非高亮需处置字面报告六项（D-10/D-24）：
    ①死子节候选 ②空段落图片位 ③措辞候选 ④第 5 章静态枚举 ⑤空浮动 VML
    文本框 ⑥页眉页脚/封面残留字面。"""
    literal_records = []   # type: List[Dict[str, Any]]
    report = []            # type: List[Dict[str, Any]]
    # XXX 字面（封面标题表+结论句，D-24②：未录入=保留原文）
    occ = 0
    total = 0
    for b in blocks:
        total += b.text.count(u"XXX")
    for b in blocks:
        for m in re.finditer(u"XXX", b.text):
            occ += 1
            literal_records.append({
                "id": "lit_%03d" % len(literal_records), "block": b.id,
                "text": u"XXX", "occurrence": occ, "phrase_total": total,
                "chapter": b.chapter, "suggest_bind": "manual_fields.project_subject",
                "category": u"非高亮字面（D-24②）：文本槽 occurrence=all 绑 project_subject",
            })
    # ① 死子节候选：子节标题 + 体≈"（同上）/同上"
    for i, b in enumerate(blocks):
        if not b.is_subsection:
            continue
        body_texts = []
        for nb in blocks[i + 1:]:
            if nb.is_subsection or nb.is_heading:
                break
            body_texts.append(nb.text.strip())
        joined = "".join(body_texts)
        if joined and joined.replace(u"（同上）", "").replace(u"同上", "").strip() == "":
            report.append({"item": u"死子节候选", "block": b.id,
                           "text": b.heading_text + u" / " + joined,
                           "suggest": u"L3 exclude_subsections 剔除（U-05）"})
    # ② 空段落图片位（题注"图N"前的纯空段；排除含图/VML/高亮）
    for i, b in enumerate(blocks):
        if b.kind != "para" or not re.match(r"^图\d+ ", b.text.strip()):
            continue
        prev = blocks[i - 1] if i > 0 else None
        if prev is None or prev.kind != "para" or prev.text.strip():
            continue
        px = prev.xml()
        if "<w:drawing" in px or "<w:pict" in px or "<w:object" in px \
                or "w:highlight" in px:
            continue
        report.append({"item": u"空段落图片位", "block": prev.id,
                       "text": u"（题注 %r 前）" % b.text.strip()[:20],
                       "suggest": u"图片槽形态 2（block_paragraph 坐标锚点，人工确认）"})
    # ③ 措辞候选（结论句/小结句）
    for b in blocks:
        t = b.text.strip()
        if t.startswith(u"通过有限元计算分析"):
            report.append({"item": u"措辞候选（结论句）", "block": b.id, "text": t[:40],
                           "suggest": u"conditional_text 槽（match_prefix）"})
        if t.startswith(u"综合上所述"):
            report.append({"item": u"措辞候选（小结句）", "block": b.id, "text": t[:40],
                           "suggest": u"可选第二条件措辞槽（D-24⑤：未声明=原样保留+提示）"})
    # ④ 第 5 章静态枚举（keep_and_warn）
    for b in blocks:
        if b.chapter and re.match(r"^\d+）", b.text.strip()):
            report.append({"item": u"静态工况枚举", "block": b.id, "text": b.text.strip()[:20],
                           "suggest": u"keep_and_warn（U-05 暂定口径：N≠3 时陈旧属已知残留）"})
    # ⑤ 空浮动 VML 文本框登记
    for b in blocks:
        for pm in re.finditer(r"<w:pict\b.*?</w:pict>", b.xml(), re.S):
            seg = pm.group(0)
            txt = tdroox.fragment_text(seg).strip()
            if not txt:
                report.append({"item": u"空浮动 VML 文本框", "block": b.id,
                               "text": u"（空文本框，untouched 自然保留）",
                               "suggest": u"登记防误清理（D-24④）"})
    # ⑥ 封面残留字面（0-0JW；页眉页脚残留由 hf 扫描 warn 承担）
    for b in blocks:
        if u"0-0JW" in b.text:
            report.append({"item": u"封面残留字面 0-0JW", "block": b.id, "text": b.text.strip()[:30],
                           "suggest": u"登记+digest warn（C3-04/H-03：首期不改写，模板修订建议）"})
    literal_records.extend(report)   # 报告项并入 index 输出便于检索
    return literal_records


def _feature_scan(doc, blocks):
    # type: (str, List[Block]) -> List[str]
    """S5-1 模板特性清单 warn（嵌套表格/OMML/图表 SmartArt/多分栏/竖排/
    脚注占位/w:sdt/fldSimple/高亮跨度内 w:br,w:tab——本模板实测全 0）"""
    warns = []
    for bi, blk in enumerate(blocks):
        if blk.kind != "table":
            continue
        inner = len(re.findall(r"<w:tbl>", blk.xml())) - 1
        if inner:
            warns.append(u"块 %s 含嵌套表格（S5-1 特性清单）" % blk.id)
    if re.search(r"<m:oMath[ >]", doc):
        warns.append(u"检测到 OMML 公式（S5-1）")
    if re.search(r"<w:sdt[ >]", doc):
        warns.append(u"检测到 w:sdt 内容控件包裹（S5-1）")
    if re.search(r"<w:fldSimple", doc):
        warns.append(u"检测到 w:fldSimple 简单域（R3-05：不在域硬边界规则内，防御 warn）")
    for sm in re.finditer(r"<w:cols[^>]*w:num=\"(\d+)\"", doc):
        if int(sm.group(1)) > 1:
            warns.append(u"检测到多分栏（S5-1）")
    if re.search(r'<w:textDirection w:val="(tbRl|btLr)"', doc):
        warns.append(u"检测到竖排文本（S5-1）")
    if re.search(r"<w:footnoteReference", doc):
        warns.append(u"检测到脚注占位引用（S5-1）")
    # 高亮 run 内 w:br/w:tab（CA-04：此类 run 不参与跨度归并与跨界拼接——
    # 跨度永不跨越拼接边界；文本段处置需人工确认）
    for blk in blocks:
        hit_break = False
        for r in tdroox.iter_runs(blk.xml()):
            if r.highlight and r.has_text and r.has_break:
                hit_break = True
                break
        if hit_break:
            warns.append(u"块 %s 高亮 run 内含 w:br/w:tab（S5-1：拼接边界，"
                         u"该 run 文本不参与跨度归并与字面跨界拼接，需人工确认）" % blk.id)
    return warns


def _dump_rules_json(draft):
    # type: (Dict[str, Any]) -> bytes
    """规则草稿落盘：JSON+整行 # 注释头（编写蓝本；strip_line_comments 兼容）
    RC-01：文件首行自声明注释约定（标准 JSON 解析器需先剥整行 # 注释）"""
    head = (
        "# 本文件含整行 # 注释（JSONC 约定）：由 GUI/命令行工具剥离注释后解析，\n"
        "# 标准 JSON 解析器直接解析会报错；手工编辑请保留该约定（不支持行尾注释）。\n"
        "# 规则层候选草稿（digest 自动生成）——人工确认检查单（D-18）把关后定版：\n"
        "# □键语义×槽语义对应 □occurrence 对号（对照消化报告 hl_001…）□格式化规则\n"
        "# □死子节剔除与字面改写声明 □表格列键与表头策略 □措辞分支与标缺文案 □OLE 处置态\n"
        "# 整行 # 注释在加载时剥除；不支持行尾注释（03 §2.2）。\n")
    body = json.dumps(draft, ensure_ascii=False, indent=1)
    return (head + body).encode("utf-8")


def _now():
    # type: () -> str
    import datetime
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def _render_report_text(report):
    # type: (Dict[str, Any]) -> str
    lines = []
    st = report["stats"]
    # DX-15：报告头导读（每节一句话+下一步动作指引）
    lines.append(u"== 消化报告：%s ==" % report["template_id"])
    lines.append(u"导读：候选清单=模板内高亮占位与槽声明的对账（需人工确认）；"
                 u"非高亮字面报告=不需建槽但需处置的模板字面；warn 节=模板特性与残留登记。")
    lines.append(u"下一步：规则层草稿可用命令行工具取出/回填编辑（get 取出→人工修改→put 回填），"
                 u"或使用导入向导步③编辑引导。")
    lines.append(u"容器：%s" % report["container"])
    lines.append(u"部件对账：%d/%d 有去向 | 块数=%d | 章=%d | 表=%d | 节=%d"
                 % (st["parts_dispositioned"], st["source_parts"], st["blocks"],
                    st["chapters"], st["tables"], st["sections"]))
    lines.append(u"域：fldChar begin=%d（instrText %d）| 书签=%d（游标 %d）| OLE=%d | PAGEREF 死条目=%d"
                 % (st["fields_fldchar_begin"], st["fields_instrtext"], st["bookmarks"],
                    st["cursor_bookmarks"], st["ole_objects"],
                    len(report["fields_dead_pageref"])))
    acc = st["highlight_accounting"]
    lines.append(u"高亮会计学：%d 总 = %d 文本run + %d 域字符run + %d 空run + %d 段落标记"
                 u" + %d br/tab排除run"
                 % (st["highlights_total_in_blocks"], acc["text_runs_in_spans"],
                    acc["field_char_runs"], acc["empty_wt_runs"], acc["para_marks"],
                    acc.get("break_excluded_runs", 0)))
    lines.append(u"归并：跨度=%d | 唯一短语=%d | 建议命中=%d（%.0f%%）"
                 % (st["highlight_spans"], st["unique_phrases"], st["suggest_hits"],
                    st["suggest_rate"] * 100))
    sm = report.get("suggest_table") or {}
    lines.append(u"建议表：%s%s（T0a 伴生覆盖内置，归一化匹配；哈希 %s）"
                 % (sm.get("source", "builtin"),
                    ("：" + sm["companion"]) if sm.get("companion") else u"（无伴生文件）",
                    sm.get("sha1") or u"-"))
    lines.append(u"引擎托管子跨度（SEQ 缓存/题注工况名）：%d" % st["engine_managed"])
    lines.append(u"---- 候选清单（人工确认，引擎托管项已排除 S5-4）----")
    for c in report["candidates"]:
        if c["in_field_result"]:
            continue
        lines.append(u"%s blk=%s occ=%s [%s] %r => %s"
                     % (c["id"], c["block"], c["occurrence"], c["managed"],
                        c["text"], c.get("suggest_bind")))
    lines.append(u"---- 非高亮需处置字面报告（D-10/D-24 六项）----")
    for r in report["literal_report"]:
        if "item" in r:
            lines.append(u"[%s] %s：%s => %s" % (r["item"], r["block"], r["text"], r["suggest"]))
    if report["fields_dead_pageref"]:
        lines.append(u"---- PAGEREF 死条目（已知残留，刷新自愈 H-10）----")
        for d in report["fields_dead_pageref"]:
            lines.append(u"%s @%s" % (d["instr"], d["block"]))
    if report["warnings"]:
        lines.append(u"---- warn（%d）----" % len(report["warnings"]))
        for w in report["warnings"]:
            lines.append(u"! %s" % w)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# verify_roundtrip（FR-31①：提炼产物基准——T-01-⑤ 口径）
# ---------------------------------------------------------------------------

def verify_roundtrip(src_docx_path, container_path):
    # type: (str, str) -> Dict[str, Any]
    """解包容器与源 docx：资产化部件逐字节 diff + manifest 对账 30 部件
    全有去向核对。返回 DiffReport（ok/明细）。"""
    src = tdroox.read_package(src_docx_path)
    con = tdroox.read_package(container_path)
    manifest = json.loads(con["manifest.json"].decode("utf-8"))
    diffs = []
    for p in manifest["parts"]:
        sp, disp, store = p["source_part"], p.get("disposition"), p.get("store")
        if disp == "asset":
            if store not in con:
                diffs.append({"part": sp, "issue": u"容器缺少资产部件 %s" % store})
            elif con[store] != src.get(sp):
                diffs.append({"part": sp, "issue": u"资产部件与源逐字节不一致"})
        # content_blocks / engine_rebuild 不比对字节（T-01-⑤）
    missing_disp = [p["source_part"] for p in manifest["parts"] if not p.get("disposition")]
    return {
        "ok": not diffs and not missing_disp,
        "byte_diffs": diffs,
        "parts_without_disposition": missing_disp,
        "parts_total": len(manifest["parts"]),
    }


# ---------------------------------------------------------------------------
# upgrade_container（R11：重消化+新旧 rules 结构化 diff）
# ---------------------------------------------------------------------------

def upgrade_container(src_docx_path, old_container, out_dir):
    # type: (str, str, str) -> Dict[str, Any]
    """重消化出新容器 + 新旧 rules.json 结构化 diff 报告（对齐键=六层结构
    路径，非短语字面——D-20）；首期不做自动合并，人工合并后强制重走
    02 §2.3 步骤 3~6。
    CA-03：先读旧容器（rules 基线+manifest 基线）再消化新 docx——旧容器
    字节只读；消化前预判输出落点与旧容器同路径即阻断（提示换输出目录），
    人工确认定版的旧容器绝不被覆盖、diff 不再退化为自比。"""
    old_con = tdroox.read_package(old_container)
    old_rules = json.loads(strip_jsonc(old_con["rules/rules.json"].decode("utf-8")))
    old_manifest = json.loads(old_con["manifest.json"].decode("utf-8"))
    # 输出落点预判（与 digest 同名规则一致）：<out_dir>/<template_id>.rtdoc
    template_id = re.sub(r"[^0-9A-Za-z\-_]+", "-", os.path.splitext(
        os.path.basename(src_docx_path))[0]).strip("-").lower() or "template"
    predicted = os.path.abspath(os.path.join(out_dir, template_id + ".rtdoc"))
    if predicted == os.path.abspath(old_container):
        raise DigestBlocked(
            u"升级输出将覆盖旧容器（%s）——CA-03 禁止覆盖人工定版容器，"
            u"请改用其他输出目录后重试" % old_container)
    report = digest(src_docx_path, out_dir)
    new_con = tdroox.read_package(report["container"])
    new_rules = json.loads(strip_jsonc(new_con["rules/rules.json"].decode("utf-8")))
    diff = _rules_diff(old_rules, new_rules)
    diff_path = report["container"].replace(".rtdoc", "_upgrade_diff.json")
    with io.open(diff_path, "w", encoding="utf-8") as f:
        f.write(json.dumps(diff, ensure_ascii=False, indent=1))
    report["upgrade_diff"] = diff
    report["upgrade_diff_path"] = diff_path
    report["upgrade_old_template_id"] = old_manifest.get("template_id")
    return report


def strip_jsonc(text):
    # type: (str) -> str
    return tdrrules.strip_line_comments(text)


def _rules_diff(old, new):
    # type: (Dict[str, Any], Dict[str, Any]) -> Dict[str, Any]
    """六层结构路径对齐 diff（added/removed/changed 逐条）"""
    out = {"added": [], "removed": [], "changed": []}

    def walk(a, b, path):
        # type: (Any, Any, str) -> None
        if isinstance(a, dict) and isinstance(b, dict):
            for k in sorted(set(a.keys()) | set(b.keys())):
                if k not in a:
                    out["added"].append({"path": path + "." + str(k)})
                elif k not in b:
                    out["removed"].append({"path": path + "." + str(k)})
                else:
                    walk(a[k], b[k], path + "." + str(k))
        elif isinstance(a, list) and isinstance(b, list):
            for i in range(max(len(a), len(b))):
                pa = "%s[%d]" % (path, i)
                if i >= len(a):
                    out["added"].append({"path": pa})
                elif i >= len(b):
                    out["removed"].append({"path": pa})
                else:
                    walk(a[i], b[i], pa)
        elif a != b:
            out["changed"].append({"path": path, "old": a, "new": b})

    walk(old, new, "$")
    return out


# ---------------------------------------------------------------------------
# 命令行入口
# ---------------------------------------------------------------------------

def main(argv):
    # type: (List[str]) -> int
    if len(argv) >= 1 and argv[0] == "verify_roundtrip":
        if len(argv) != 3:
            sys.stderr.write(u"usage: tdr_digest.py verify_roundtrip <src.docx> <container>\n")
            return 2
        rep = verify_roundtrip(argv[1], argv[2])
        # DX-09：先给一句人话结论（JSON 保留为机器可读附加输出）
        if rep["ok"]:
            sys.stdout.write(u"[digest] 回读校验：通过（%d 部件对账全部有去向，"
                             u"资产逐字节一致）\n" % rep["parts_total"])
        else:
            sys.stdout.write(u"[digest] 回读校验：未通过（差异 %d 处，明细见下方 JSON）\n"
                             % len(rep["byte_diffs"]))
        sys.stdout.write(json.dumps(rep, ensure_ascii=False, indent=1) + "\n")
        return 0 if rep["ok"] else 1
    if len(argv) >= 1 and argv[0] == "upgrade_container":
        if len(argv) != 4:
            sys.stderr.write(u"usage: tdr_digest.py upgrade_container <src.docx> <old.rtdoc> <out_dir>\n")
            return 2
        try:
            rep = upgrade_container(argv[1], argv[2], argv[3])
        except DigestBlocked as e:
            sys.stderr.write(u"[digest] 升级阻断：%s\n" % e.reason)
            return 1
        sys.stdout.write(json.dumps({"container": rep["container"],
                                     "upgrade_diff": rep.get("upgrade_diff")},
                                    ensure_ascii=False, indent=1) + "\n")
        return 0
    # T0a：--suggest 可选参数（与既有位置参数风格并存，缺省=同名自动发现）
    suggest_path = None   # type: Optional[str]
    args = list(argv)
    if "--suggest" in args:
        i = args.index("--suggest")
        if i + 1 >= len(args):
            sys.stderr.write(u"usage error: --suggest 需要文件路径参数\n")
            return 2
        suggest_path = args[i + 1]
        del args[i:i + 2]
    if len(args) != 2:
        sys.stderr.write(u"usage: tdr_digest.py <src.docx> <out_dir> [--suggest <path>]\n"
                         u"       tdr_digest.py verify_roundtrip <src.docx> <container>\n"
                         u"       tdr_digest.py upgrade_container <src.docx> <old.rtdoc> <out_dir>\n"
                         u"退出码：0=成功，1=失败（详见输出），2=用法错误\n")
        return 2
    src, out_dir = args[0], args[1]
    try:
        report = digest(src, out_dir, suggest_path)
    except DigestBlocked as e:
        # DX-05：单流输出（GUI 按本行协议解析；stderr 重复份/BLOCKED 英文标记撤除）
        sys.stdout.write(u"[digest] 前置校验阻断：%s\n" % e.reason)
        return 1
    st = report["stats"]
    sys.stdout.write(u"[digest] 完成：容器=%s\n" % report["container"])
    sys.stdout.write(u"[digest] 部件对账 %d/%d | 块=%d | 高亮跨度=%d 唯一短语=%d | warn=%d\n"
                    % (st["parts_dispositioned"], st["source_parts"], st["blocks"],
                       st["highlight_spans"], st["unique_phrases"], len(report["warnings"])))
    # DX-08：完成行给出报告文件路径（warn 明细与编辑中提示只在报告内）
    report_txt = os.path.splitext(report["container"])[0] + "_digest_report.txt"
    sys.stdout.write(u"[digest] 报告=%s（含 %d 条 warn 明细，见报告尾节）\n"
                     % (report_txt, len(report["warnings"])))
    return 0


if __name__ == "__main__":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8")
    sys.exit(main(sys.argv[1:]))
