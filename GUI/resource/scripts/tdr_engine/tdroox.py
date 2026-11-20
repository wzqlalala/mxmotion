#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""tdroox.py——模板化 DOCX 报告引擎的 OOXML 底层操作库（纯标准库，python 3.7 兼容）。

手法移植自既有 export_docx.py（冻结只读借鉴源，函数级出处行号见各 docstring），
脚本本身零修改；本模块为其手法在新引擎中的落地形态，并按 03-技术路线 §4.1 扩展：
  - MediaRegistry 媒体键扫描正则与落包路径参数化（A-06：移植源硬编码 word/media 前缀）；
  - ensure_content_type_override/Default 映射表扩展 MSFT 扩展类型
    （T-01-④：本模板实测 Default bin=…attachedToolbars、
     Override /word/customizations.xml=…keyMapCustomizations+xml，移植源无此先例）；
  - 跨 run 归并/匹配的 run 级迭代与域硬边界工具（03 §4.2-② R-a：fldChar/instrText
    一律为归并硬边界——digest 高亮跨度与引擎字面改写共用同一套边界语义）。

工程约束（03 §0）：全部代码兼容 python 3.7——禁 walrus/match/内建泛型下标注解
（签名一律 typing.Dict/List/Tuple）/f-string "=" 调试语法；所有 open() 显式
encoding="utf-8"；替换值写入 w:t 一律带 xml:space="preserve"（S5-1，模板 114 处先例）。
"""

import os
import re
import zipfile
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as _sax_escape

# 1 twip = 635 EMU（移植 export_docx.py:69）；版心宽度兜底值（移植 :71）
TWIP_TO_EMU = 635
DEFAULT_BODY_WIDTH_EMU = 6645910
DEFAULT_BODY_HEIGHT_TWIP = 15398

# Content_Types Default 扩展名映射（移植 export_docx.py:100-105）+ MSFT 扩展类型
# （T-01-④：attachedToolbars.bin 的 Default 与 customizations.xml 的 Override
# 必须被映射表覆盖，漏登记即 Word/WPS 修复对话框——C-01/NFR-4）
MEDIA_CONTENT_TYPES = {
    ".png": "image/png", ".jpeg": "image/jpeg", ".jpg": "image/jpeg",
    ".gif": "image/gif", ".bmp": "image/bmp", ".tif": "image/tiff",
    ".tiff": "image/tiff", ".emf": "image/x-emf", ".wmf": "image/x-wmf",
    ".bin": "application/vnd.ms-word.attachedToolbars",
}

# 自定义 XML 部件的 Content_Types Override 映射（T-01-④ 扩展登记）
PART_CONTENT_TYPES = {
    "/word/customizations.xml":
        "application/vnd.ms-word.keyMapCustomizations+xml",
}

# MSFT 扩展部件的包级关系类型（M1 rels 闭包校验时用于识别 customizations 链）
# 无需特判：闭包校验按 Target 存在性通用核对，这里仅作文档注记。


def xml_escape(s):
    # type: (str) -> str
    """E8 统一 XML 转义：& < > 与属性引号全覆盖（值只写入元素文本，引号
    转义冗余但无害——保证任何绑定值路径都安全）。"""
    return _sax_escape(s, {"\"": "&quot;", "'": "&apos;"})


def read_package(path):
    # type: (str) -> Dict[str, bytes]
    """读 zip 包全部非目录条目为 {name: bytes}（移植 export_docx.py:108）"""
    pkg = {}
    with zipfile.ZipFile(path, "r") as z:
        for info in z.infolist():
            if info.filename.endswith("/"):
                continue
            pkg[info.filename] = z.read(info.filename)
    return pkg


class PackageLimitError(Exception):
    """输入超限（CA-05/CA-08/S5-3：中央目录预检或限量流式读失败）。
    message 为可读文案（调用方直转 EngineError/DigestBlocked）。"""


def zip_preflight_z(z, limits):
    # type: (zipfile.ZipFile, Dict[str, int]) -> Tuple[int, int]
    """零解压预检（按中央目录 compress_size/file_size，不解压任何条目——
    CA-05：内存峰值先于检查发生的问题由此闭合）。limits 键：max_parts/
    max_part_bytes/max_total_bytes/max_ratio。返回 (总解压字节, 总压缩字节)。"""
    infos = [i for i in z.infolist() if not i.filename.endswith("/")]
    if len(infos) > int(limits["max_parts"]):
        raise PackageLimitError(u"部件数 %d 超上限 %d" % (len(infos), limits["max_parts"]))
    total_u = 0
    total_c = 0
    for info in infos:
        total_u += info.file_size
        total_c += info.compress_size
        if info.file_size > int(limits["max_part_bytes"]):
            raise PackageLimitError(u"部件 %s 超单件体积上限 %d（中央目录声明 %d）"
                                    % (info.filename, limits["max_part_bytes"], info.file_size))
    if total_u > int(limits["max_total_bytes"]):
        raise PackageLimitError(u"解压总体积 %d 超上限 %d" % (total_u, limits["max_total_bytes"]))
    if total_c > 0 and total_u * 1.0 / total_c > float(limits["max_ratio"]):
        raise PackageLimitError(u"解压比率 %.0f 超上限 %d"
                                % (total_u * 1.0 / total_c, int(limits["max_ratio"])))
    return total_u, total_c


def zip_preflight(path, limits):
    # type: (str, Dict[str, int]) -> Tuple[int, int]
    """zip_preflight_z 的按路径形态（内部开闭 zip）。"""
    with zipfile.ZipFile(path, "r") as z:
        return zip_preflight_z(z, limits)


def read_package_z_limited(z, limits):
    # type: (zipfile.ZipFile, Dict[str, int]) -> Dict[str, bytes]
    """限量流式读包（CA-05）：逐条目 z.open 限量读取（超过单件上限即断，
    不信任中央目录声明的 file_size）；调用方应先 zip_preflight 做总体积/比率/
    部件数预检。"""
    cap = int(limits["max_part_bytes"])
    pkg = {}   # type: Dict[str, bytes]
    for info in z.infolist():
        if info.filename.endswith("/"):
            continue
        with z.open(info) as f:
            data = f.read(cap + 1)
        if len(data) > cap:
            raise PackageLimitError(u"部件 %s 超单件体积上限 %d（实际读入超限）"
                                    % (info.filename, cap))
        pkg[info.filename] = data
    return pkg


def read_entry_limited(path, name, cap):
    # type: (str, str, int) -> Optional[bytes]
    """按名限量读取单条目（不存在返回 None）——manifest 先读的两阶段限量
    读包场景（CA-05/CA-08）；超过 cap 抛 PackageLimitError。"""
    with zipfile.ZipFile(path, "r") as z:
        try:
            info = z.getinfo(name)
        except KeyError:
            return None
        with z.open(info) as f:
            data = f.read(cap + 1)
    if len(data) > cap:
        raise PackageLimitError(u"部件 %s 超读取上限 %d" % (name, cap))
    return data


def read_package_limited(path, limits):
    # type: (str, Dict[str, int]) -> Dict[str, bytes]
    """read_package_z_limited 的按路径形态（预检+限量读一次完成）。"""
    zip_preflight(path, limits)
    with zipfile.ZipFile(path, "r") as z:
        return read_package_z_limited(z, limits)


def write_package(parts, out_path):
    # type: (Dict[str, bytes], str) -> None
    """组包落盘（DEFLATED；docx 允许）。全部条目写字节；调用方（引擎层
    finish_atomic）负责先 tmp 后 rename 的原子语义与失败清理（T-13/A-01）。
    注意：此处不落盘文本——所有 open 语义由 zipfile 内部处理，部件内容
    一律 UTF-8 字节（T-18：文件 IO 显式编码在 digest/engine 的 json/txt
    写出处体现）。"""
    with zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as z:
        for name in sorted(parts.keys()):
            z.writestr(name, parts[name])


def png_pixel_size(path):
    # type: (str) -> Optional[Tuple[int, int]]
    """读 PNG IHDR 取像素宽高（移植 export_docx.py:119）；非 PNG/解析失败返回 None"""
    try:
        with open(path, "rb") as f:
            head = f.read(24)
        if len(head) < 24 or head[:8] != b"\x89PNG\r\n\x1a\n" or head[12:16] != b"IHDR":
            return None
        w = int.from_bytes(head[16:20], "big")
        h = int.from_bytes(head[20:24], "big")
        return (w, h) if w > 0 and h > 0 else None
    except Exception:
        return None


# 图片格式魔数（E7/BUG-02/C12-Q-03：扩展名不作判据，首期支持 PNG/JPEG/BMP）
_PNG_MAGIC = b"\x89PNG\r\n\x1a\n"
_JPEG_SOF_MARKERS = (0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
                     0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF)
# 首期不支持但属已知图片格式的魔数（供告警文案区分"不支持的图片格式"/"非图片字节"）
_OTHER_IMAGE_MAGIC = ((b"GIF87a", "gif"), (b"GIF89a", "gif"),
                      (b"II*\x00", "tiff"), (b"MM\x00*", "tiff"),
                      (b"\x00\x00\x01\x00", "ico"), (b"\x00\x00\x02\x00", "cur"))


def probe_image(path):
    # type: (str) -> Tuple[Optional[str], Optional[Tuple[int, int]]]
    """图片文件双校验（BUG-02/E7，C12-Q-03 口径）：文件头魔数识别格式+
    可解析像素尺寸，两关皆过才算可用图片；扩展名一律不作判据。
    返回 (格式名, (宽, 高))：
      - 格式 None = 头部不属任何已知图片格式（"非图片字节"）；
      - 格式=png/jpeg/bmp 且尺寸非 None = 通过双校验（首期支持三格式）；
      - 格式=png/jpeg/bmp 但尺寸 None = 头可识别而尺寸不可解析（同按阻断）；
      - 格式=gif/tiff/ico/cur = 已知图片格式但不在首期支持列表（C12：与
        伪图片同路径阻断，文案按"不支持的图片格式"区分），尺寸不再解析。
    """
    try:
        with open(path, "rb") as f:
            head = f.read(65536)
    except Exception:
        return None, None
    if head[:8] == _PNG_MAGIC:
        size = _png_head_size(head)
        return ("png", size) if size else ("png", None)
    if head[:2] == b"\xff\xd8":
        size = _jpeg_head_size(head)
        return ("jpeg", size) if size else ("jpeg", None)
    if head[:2] == b"BM":
        if len(head) >= 26:
            w = int.from_bytes(head[18:22], "little")
            h = int.from_bytes(head[22:26], "little")
            if w > 0 and h != 0:
                return ("bmp", (w, abs(h)))   # h<0=顶向下 DIB，尺寸仍有效
            return ("bmp", None)
        return ("bmp", None)
    for magic, name in _OTHER_IMAGE_MAGIC:
        if head[:len(magic)] == magic:
            return (name, None)
    return None, None


def _png_head_size(head):
    # type: (bytes) -> Optional[Tuple[int, int]]
    """头字节内解析 PNG IHDR 宽高（与 png_pixel_size 同判据）"""
    if len(head) < 24 or head[12:16] != b"IHDR":
        return None
    w = int.from_bytes(head[16:20], "big")
    h = int.from_bytes(head[20:24], "big")
    return (w, h) if w > 0 and h > 0 else None


def _jpeg_head_size(data):
    # type: (bytes) -> Optional[Tuple[int, int]]
    """扫描 JPEG 段链定位 SOFn 取维度（常规编码器 SOF 均在头部 64KB 内；
    未找到/段长非法=None。standalone/填充段按规范跳过）"""
    n = len(data)
    i = 2
    while i + 4 <= n:
        if data[i] != 0xFF:
            i += 1
            continue
        marker = data[i + 1]
        if marker in (0xFF, 0x00, 0x01) or 0xD0 <= marker <= 0xD7:
            i += 2
            continue
        if marker in _JPEG_SOF_MARKERS:
            if i + 9 > n:
                return None
            h = int.from_bytes(data[i + 5:i + 7], "big")
            w = int.from_bytes(data[i + 7:i + 9], "big")
            return (w, h) if w > 0 and h > 0 else None
        seg_len = int.from_bytes(data[i + 2:i + 4], "big")
        if seg_len < 2:
            return None
        i += 2 + seg_len
    return None


# ---------------------------------------------------------------------------
# document.xml 顶层结构解析（移植 export_docx.py:137-209）
# ---------------------------------------------------------------------------

_FIRST_TAG = re.compile(r"<(w:p|w:tbl|w:sdt|w:sectPr)\b[^>]*?(/?)>")

# 注释/CDATA/处理指令区段（CC-04/E5：区内标签不得当真元素解析、区内文本
# 不得参与匹配与替换；未闭合形态兜底到串尾，由落包前 wf 自检阻断）
_OPAQUE_PAT = re.compile(
    r"<!--.*?-->|<!\[CDATA\[.*?\]\]>|<\?.*?\?>|<!--.*$|<!\[CDATA\[.*$", re.S)


def mask_opaque(xml):
    # type: (str) -> str
    """注释/CDATA/PI 区段内容置空白（等长替换，字节偏移不变——CC-04）：
    结构扫描（top_level_children/child_spans）与文本拼接（fragment_text）
    先经本函数，注释内 <w:tbl>/<w:t> 不再被当真元素/真文本；定界符本身
    保留（非标签形态，扫描器自然跳过）。"""
    if "<!--" not in xml and "<![CDATA[" not in xml and "<?" not in xml:
        return xml

    def _blank(m):
        # type: (Any) -> str
        s = m.group(0)
        if s.startswith("<!--"):
            head = "<!--"
            tail = "-->" if s.endswith("-->") else ""
        elif s.startswith("<![CDATA["):
            head = "<![CDATA["
            tail = "]]>" if s.endswith("]]>") else ""
        else:
            head = "<?"
            tail = "?>" if s.endswith("?>") else ""
        return head + (" " * (len(s) - len(head) - len(tail))) + tail
    return _OPAQUE_PAT.sub(_blank, xml)


def top_level_children(body):
    # type: (str) -> List[Tuple[str, int, int]]
    """w:body 内顶层元素片段 [(tag, start, end)]（按文档顺序；嵌套同名标签
    按深度配对，正文里散落的书签等其余标签跳过；注释/CDATA/PI 区段不参与
    配对——CC-04，返回偏移对原文有效）（移植 export_docx.py:140）"""
    masked = mask_opaque(body)
    children = []
    i = 0
    n = len(masked)
    while i < n:
        lt = masked.find("<", i)
        if lt < 0:
            break
        m = _FIRST_TAG.match(masked, lt)
        if not m:
            i = lt + 1
            continue
        tag = m.group(1)
        if m.group(2):   # 自闭合
            children.append((tag, lt, m.end()))
            i = m.end()
            continue
        pat = re.compile(r"<(/?)%s\b[^>]*?(/?)>" % tag)
        depth = 1
        end = n
        for mm in pat.finditer(masked, m.end()):
            if mm.group(2):
                continue
            if mm.group(1) == "":
                depth += 1
            else:
                depth -= 1
                if depth == 0:
                    end = mm.end()
                    break
        children.append((tag, lt, end))
        i = end
    return children


def child_spans(frag, tag):
    # type: (str, str) -> List[Tuple[int, int]]
    """frag 内顶层 tag 元素配对片段（移植 export_docx.py:181 _child_spans；
    单元格内嵌套表的同名标签不误配；注释/CDATA 区段不参与配对——CC-04；
    供行/单元格级定位复用，返回偏移对原文有效）"""
    masked = mask_opaque(frag)
    spans = []
    pat = re.compile(r"<(/?)%s\b[^>]*?(/?)>" % tag)
    i, n = 0, len(masked)
    while i < n:
        m = pat.search(masked, i)
        if not m:
            break
        if m.group(1):      # 孤立闭合标签（防御性跳过）
            i = m.end()
            continue
        if m.group(2):      # 自闭合
            spans.append((m.start(), m.end()))
            i = m.end()
            continue
        depth = 1
        end = n
        for mm in pat.finditer(masked, m.end()):
            if mm.group(2):
                continue
            depth += 1 if mm.group(1) == "" else -1
            if depth == 0:
                end = mm.end()
                break
        spans.append((m.start(), end))
        i = end
    return spans


def fragment_text(frag):
    # type: (str) -> str
    """片段内全部 w:t 文本拼接（移植 export_docx.py:212 _fragment_text；
    注释/CDATA 内伪 w:t 不参与——CC-04 命中不替换）"""
    return "".join(re.findall(r"<w:t[^>]*>([^<]*)</w:t>", mask_opaque(frag)))


# ---------------------------------------------------------------------------
# run 级迭代与域硬边界（03 §4.2-② R-a / §4.4 R-b/R-d 的共用底层语义）
# ---------------------------------------------------------------------------

# 自闭合 <w:r/> 必须优先于配对分支（CA-07：旧形态 `\b` 在 "r"/"/" 间成立，
# 自闭合 run 被配对分支 lazy 吞至下一个 </w:r>，run 边界/rPr 归属失真）
_RUN_PAT = re.compile(r"<w:r\b[^>]*/>|<w:r\b[^>]*>.*?</w:r>", re.S)

# run 内文本 token（w:t）与拼接边界（w:br/w:tab）的文档序扫描（CA-04：
# 供 breaks 字符坐标计算——br/tab 在 run 拼接文本内的落点即匹配边界）
_RUN_TOKEN_PAT = re.compile(
    r"<w:t[^>]*>([^<]*)</w:t>|<w:br\b[^>]*?/?>|<w:tab\b[^>]*?/?>")


class RunInfo(object):
    """单个 w:r 的轻量视图（字节区间 + 文本 + 高亮/域标记）。
    text：w:t 文本拼接（未反转义，OOXML 文本内实体保持原样——匹配按原样
    字面进行，中文占位不含实体）；has_text：存在非空 w:t；
    highlight：rPr 含 w:highlight；fld/instr：域字符/指令标记；
    has_break：run 内容含 w:br 或 w:tab（拼接边界，S5-1）；
    breaks：各 br/tab 在 text 拼接坐标内的落点（CA-04：跨 br/tab 的字面
    命中被拒绝——文本可参与拼接匹配，但命中不得跨越拼接边界）。"""

    __slots__ = ("start", "end", "text", "has_text", "highlight",
                 "fld", "instr", "has_break", "object", "breaks")

    def __init__(self, start, end, text, has_text, highlight, fld, instr,
                 has_break, object_, breaks):
        self.start = start          # type: int
        self.end = end              # type: int
        self.text = text            # type: str
        self.has_text = has_text    # type: bool
        self.highlight = highlight  # type: bool
        self.fld = fld              # type: Optional[str]  # begin/separate/end
        self.instr = instr          # type: Optional[str]
        self.has_break = has_break  # type: bool
        self.object = object_       # type: bool  # 含 w:object（OLE，T-22 互斥）
        self.breaks = breaks        # type: Tuple[int, ...]


def iter_runs(frag):
    # type: (str) -> List[RunInfo]
    """按文档序迭代片段内全部 w:r（含嵌套上下文中的 run——调用方按块/段
    粒度传入，模板实测无文本框嵌套占位，防御面由 S5-1 特性 warn 承担）"""
    out = []
    for m in _RUN_PAT.finditer(frag):
        r = m.group(0)
        fld_m = re.search(r'w:fldCharType="(\w+)"', r)
        instrs = re.findall(r"<w:instrText[^>]*>([^<]*)</w:instrText>", r)
        ts = re.findall(r"<w:t[^>]*>([^<]*)</w:t>", r)
        text = "".join(ts)
        breaks = []   # type: List[int]
        acc = 0
        for tm in _RUN_TOKEN_PAT.finditer(r):
            if tm.group(1) is not None:
                acc += len(tm.group(1))
            else:
                breaks.append(acc)
        out.append(RunInfo(
            m.start(), m.end(), text, bool(text.strip()) or bool(text),
            ("w:highlight" in r),
            fld_m.group(1) if fld_m else None,
            "".join(instrs) if instrs else None,
            ("<w:br" in r or "<w:tab" in r),
            "<w:object" in r,
            tuple(breaks),
        ))
    return out


def para_mark_highlight(para_xml):
    # type: (str) -> bool
    """段落标记（pPr/rPr）是否带高亮（T-22：清高亮断言双处之一，实测 20 处）"""
    m = re.search(r"<w:pPr>.*?</w:pPr>", para_xml, re.S)
    if not m:
        return False
    return 'w:highlight' in m.group(0) and 'w:val="yellow"' in m.group(0)


def merge_highlight_spans(runs):
    # type: (List[RunInfo]) -> List[dict]
    """跨 run 高亮归并（03 §4.2-② R-a 域硬边界算法的底层实现）：
    - 高亮文本 run（has_text 且 highlight）连续归并为一个跨度；
    - fldChar/instrText 一律为硬边界：跨度假定封口、不并入、不跨越；
      域指令区（begin..separate）/结果区（separate..end）状态随行进更新，
      区内高亮文本 run 归并为独立子跨度并携带 in_field / in_field_result；
    - 非高亮文本 run 打断（打断即拆）；bookmarkStart/End、rsid 差异不打断
      （run 迭代根本不产出书签条目）；空 w:t 高亮 run 不并入文本（其归属
      由 digest 单独归类为"域字符/空 run 高亮"）；
    - w:br/w:tab 视为拼接边界（S5-1，本模板实测跨度内 0 处）。
    返回 [{"start","end","text","run_range":(i0,i1),"in_field","in_field_result"}]，
    start/end 为首/末 run 的字节区间（便于引擎按区间合并重写）。"""
    spans = []
    zone = "outside"      # outside | instruction | result
    cur = None            # type: Optional[dict]
    for idx, r in enumerate(runs):
        if r.fld == "begin":
            if cur:
                spans.append(cur)
                cur = None
            zone = "instruction"
            continue
        if r.fld == "separate":
            if cur:
                spans.append(cur)
                cur = None
            zone = "result"
            continue
        if r.fld == "end":
            if cur:
                spans.append(cur)
                cur = None
            zone = "outside"
            continue
        if r.instr is not None:
            # instrText run：硬边界（域指令文本 run，模板实测其自身可带高亮
            # ——H-01：既不算高亮文本 run 也不触发打断）
            if cur:
                spans.append(cur)
                cur = None
            continue
        if cur is not None:
            # 结构性 token（空 w:t/书签等）不打破连续性——只记录游标
            contiguous = (cur["last_idx"] == idx - 1)
            cur["last_idx"] = idx
            if not contiguous:
                # 中间出现了携带文本的非本跨度 run（打断即拆）
                spans.append(cur)
                cur = None
        if r.has_break:
            # 拼接边界（S5-1/CA-04）：含 w:br/w:tab 的 run（含仅有 br/tab 的
            # run）一律封口当前跨度且不并入——跨度永不跨越 br/tab，B3 单 run
            # 合并不可能吞掉换行/制表（CA-04①②：旧实现把断点检查放在
            # has_text 之后，br-only run 不断开、text+br run 被并入后随合并
            # 丢失）。其文本不参与高亮跨度，digest 侧单独计数（hl_break_runs）
            if cur:
                spans.append(cur)
                cur = None
            continue
        if not r.has_text:
            continue
        if r.highlight:
            in_fld = zone == "instruction"
            in_res = zone == "result"
            if cur and cur["in_field"] == in_fld and cur["in_field_result"] == in_res:
                cur["end"] = r.end
                cur["text"] += r.text
                cur["run_range"] = (cur["run_range"][0], idx)
            else:
                if cur:
                    spans.append(cur)
                cur = {"start": r.start, "end": r.end, "text": r.text,
                       "run_range": (idx, idx), "in_field": in_fld,
                       "in_field_result": in_res, "last_idx": idx}
        else:
            # 非高亮文本 run：打断即拆
            if cur:
                spans.append(cur)
                cur = None
    if cur:
        spans.append(cur)
    return spans


def match_literal_in_runs(runs, phrase):
    # type: (List[RunInfo], str) -> List[Tuple[int, int, int, int]]
    """跨 run 字面匹配（03 §4.4 instantiate_loop ④ 定位机制落字：块内跨 run
    文本拼接匹配，w:br/w:tab 与域体为拼接边界）。返回命中
    [(run_start_idx, run_end_idx, char_start, char_end)]——char 坐标为**相对
    run_start_idx 文本起点**的偏移（与 rewrite_runs_with_text 的裁剪坐标系
    一致；题注隐式承接传 (lo,hi,0,len(seg)) 同约定）；仅返回"全由非高亮文本
    run 组成"的命中（R-d 域回避：literal_rewrite 仅命中非高亮、非域内文本——
    域字符/instrText 在迭代中即被排除出拼接序列）。
    CA-04③：含 br/tab 的 run 文本参与拼接，但命中区间不得跨越任何拼接边界
    落点。"""
    hits = []
    seq = []          # [(idx, RunInfo)]，当前非高亮文本 run 连续段
    seq_text = ""
    seq_off = []      # 每段 run 在 seq_text 中的起始偏移
    breaks_abs = []   # 拼接边界落点（seq_text 绝对坐标）
    for idx, r in enumerate(runs):
        boundary = (r.fld is not None or r.instr is not None
                    or not r.has_text or r.highlight)
        if boundary:
            seq, seq_text, seq_off, breaks_abs = [], "", [], []
            continue
        seq_off.append(len(seq_text))
        seq_text += r.text
        seq.append(idx)
        for bp in r.breaks:
            breaks_abs.append(seq_off[-1] + bp)
        # 在当前段内查找 phrase：仅扫描**本 run 追加后新增**的命中窗口
        # （end_at 越过追加前长度才算新命中——BUG-07：旧实现每 run 全文重扫，
        # 早期命中被重复枚举；旧消费端"取首命中+替换后重扫"掩盖了重复，
        # 快照收集模式下重复命中=同一位点被多次认领/替换）
        prev_len = seq_off[-1]
        pos = max(0, prev_len - len(phrase) + 1)
        while True:
            at = seq_text.find(phrase, pos)
            if at < 0:
                break
            end_at = at + len(phrase)
            if end_at > prev_len and not any(at < b < end_at for b in breaks_abs):
                i0 = max(i for i, off in enumerate(seq_off) if off <= at)
                i1 = max(i for i, off in enumerate(seq_off) if off < end_at)
                hits.append((seq[i0], seq[i1],
                             at - seq_off[i0], end_at - seq_off[i0]))
            pos = at + 1   # 允许重叠命中继续扫描（防御相接占位）
    return hits


def rewrite_runs_with_text(frag, run_spans, hit, new_text):
    # type: (str, List[Tuple[int, int]], Tuple[int, int, int, int], str) -> str
    """rewrite_run_range 的带值形态：把命中区间 [cs,ce) 的文本改写为 new_text
    （CA-02/CC-06：命中字符坐标参与裁剪——首 run 保留 cs 前缀、末 run 保留
    ce 后缀、中段清空；run 结构与 rPr 保持；run 内首个 w:t 承载新文本、同 run
    其余 w:t 清空）。w:t 写出恒带 xml:space="preserve"（S5-1）。"""
    i0, i1, cs, ce = hit
    run_texts = []
    for a, b in run_spans[i0:i1 + 1]:
        run_texts.append("".join(
            re.findall(r"<w:t[^>]*>([^<]*)</w:t>", frag[a:b])))
    offs = []
    acc = 0
    for t in run_texts:
        offs.append(acc)
        acc += len(t)
    out = []
    pos = 0
    for k, (a, b) in enumerate(run_spans):
        run = frag[a:b]
        out.append(frag[pos:a])
        if i0 <= k <= i1:
            t = run_texts[k - i0]
            off = offs[k - i0]
            local_cs = min(max(cs - off, 0), len(t))
            local_ce = min(max(ce - off, 0), len(t))
            if k == i0:
                head = t[:local_cs]
                tail = t[local_ce:] if k == i1 else ""
                new_txt = head + new_text + tail
            elif k == i1:
                new_txt = t[local_ce:]
            else:
                new_txt = ""
            state = [False]

            def _sub(m):
                # type: (Any) -> str
                if state[0]:
                    return m.group(1) + m.group(2)
                state[0] = True
                return m.group(1) + xml_escape(new_txt) + m.group(2)
            run = re.sub(r"(<w:t[^>]*>)[^<]*(</w:t>)", _sub, run)
        out.append(run)
        pos = b
    out.append(frag[pos:])
    return "".join(out)


# 跨包装元素边界守卫（CC-05）：合并区间内含超链接/智能标记/书签等非 run
# 包装元素时，B3 单 run 合并会吞掉其开/闭标签致 document.xml 畸形——拒绝合并，
# 调用方降级为"原样保留+告警"（E5 能降级处降级；落包前 wf 自检兜底 CC-04/05）
_WRAPPER_BOUNDARY_PAT = re.compile(
    r"<w:hyperlink\b|</w:hyperlink>|<w:smartTag\b|</w:smartTag>|"
    r"<w:bookmarkStart\b|<w:bookmarkEnd\b")


def replace_span_with_single_run(frag, run_spans, span_first, span_last, new_text):
    # type: (str, List[Tuple[int, int]], int, int, str) -> Optional[str]
    """跨度合并单 run 替换（03 §4.4 B3/R-b，限定域外子跨度）：把
    [span_first, span_last] 的 run 序列合并为单个 run——rPr 取首 run、
    清 w:highlight、w:t=xml:space="preserve" 的新文本；其余 run 删除。
    调用方保证区间内不含 fldChar/instrText（域外子跨度语义），含 w:object
    的 run 由调用方互斥排除（T-22）。区间内含超链接/书签等包装元素边界时
    返回 None（CC-05：拒绝合并不破坏结构，调用方降级+告警）。"""
    a0 = run_spans[span_first][0]
    b1 = run_spans[span_last][1]
    seg = frag[a0:b1]
    if _WRAPPER_BOUNDARY_PAT.search(seg):
        return None
    first = frag[run_spans[span_first][0]:run_spans[span_first][1]]
    rpr_m = re.search(r"<w:rPr>.*?</w:rPr>", first, re.S)
    rpr = ""
    if rpr_m:
        rpr = re.sub(r"<w:highlight[^>]*/>", "", rpr_m.group(0))
    new_run = "<w:r>%s<w:t xml:space=\"preserve\">%s</w:t></w:r>" % (
        rpr, xml_escape(new_text))
    return frag[:a0] + new_run + frag[b1:]


# ---------------------------------------------------------------------------
# 命名空间/关系/内容类型（移植 export_docx.py:419/:852/:926/:936/:950）
# ---------------------------------------------------------------------------

def add_missing_namespaces(target_xml, base_xml):
    # type: (str, str) -> str
    """把 base 根标签的 xmlns 声明并入 target 根标签（同前缀已声明跳过）
    （移植 export_docx.py:950 _add_missing_namespaces，R2b 命名空间补齐）"""
    decl = re.match(r"\s*<\?xml[^>]*\?>", target_xml)
    head_end = decl.end() if decl else 0
    troot = re.match(r"<([A-Za-z0-9:]+)\b[^>]*>", target_xml[head_end:])
    if not troot:
        return target_xml
    mroot = re.search(r"<%s\b[^>]*>" % re.escape(troot.group(1)), base_xml)
    if not mroot:
        return target_xml
    declared = set(re.findall(r"(xmlns:[A-Za-z0-9]+)=", troot.group(0)))
    add_attrs = ['%s="%s"' % (m.group(1), m.group(2))
                 for m in re.finditer(r'(xmlns:[A-Za-z0-9]+)="([^"]+)"', mroot.group(0))
                 if m.group(1) not in declared]
    if not add_attrs:
        return target_xml
    cut = head_end + troot.end() - 1     # 根标签收尾 '>' 前
    return target_xml[:cut] + " " + " ".join(add_attrs) + target_xml[cut:]


def renum_numids(xml_text, num_map):
    # type: (str, Dict[str, str]) -> str
    """全部 <w:numId w:val="N"/> 重编号（移植 export_docx.py:419；
    未命中映射→0（Word 忽略，不损坏））"""
    return re.sub(r'(<w:numId w:val=")(\d+)(")',
                  lambda m: "%s%s%s" % (m.group(1), num_map.get(m.group(2), "0"),
                                        m.group(3)),
                  xml_text)


class MediaRegistry(object):
    """媒体/关系登记器（移植 export_docx.py:852；A-06 参数化）：
    - media_dir：落包媒体目录前缀（默认 "word/media/"，兼容任意 media 目录形态）；
    - 媒体键扫描正则按 media_dir 动态构造（"word/media/image(\\d+)" 的参数化形态）；
    - rels_xml/content_types 由调用方传入现值，登记后经属性读回写包。
    扩展名→ContentType 走 MEDIA_CONTENT_TYPES（含 MSFT bin 映射，T-01-④）。"""

    def __init__(self, existing_part_names, rels_xml, content_types,
                 media_dir="word/media/"):
        # type: (List[str], str, str, str) -> None
        self.media_dir = media_dir.rstrip("/") + "/"
        self.rels_xml = rels_xml
        self.content_types = content_types
        if "</Relationships>" not in self.rels_xml:
            m = re.search(r"(<Relationships\b[^>]*?)/>\s*$", self.rels_xml)
            if m:
                self.rels_xml = m.group(1) + "></Relationships>"
        max_rid = max([int(x) for x in
                       re.findall(r'Id="rId(\d+)"', self.rels_xml)] + [0])
        key_pat = re.compile(re.escape(self.media_dir) + r"image(\d+)")
        max_img = 0
        for name in existing_part_names:
            m = key_pat.match(name)
            if m:
                max_img = max(max_img, int(m.group(1)))
        self._next_rid = max_rid + 1
        self._next_img = max_img + 1
        self._used = {}    # type: Dict[str, Tuple[str, str]]  # src_path -> (rid, media_name)
        self._bytes = {}   # type: Dict[str, bytes]              # media_name -> data
        # 注意：构造期不预补 Content_Types Default（空数据导出基准②要求未发生
        # 媒体登记时 [Content_Types].xml 逐字节一致）——Default 补登记收敛到
        # register() 内按实际扩展名幂等补（移植源 png 预补行为在此参数化收窄）

    def next_rid(self):
        # type: () -> str
        rid = "rId%d" % self._next_rid
        self._next_rid += 1
        return rid

    def _append_rel(self, rid, media_name):
        # Target 必须是**相对宿主部件（word/）的路径**（OOXML part-relative）：
        # document.xml.rels 位于 word/_rels/ 下，Target="media/x.png"→word/media/x.png；
        # 旧实现落 "word/media/x.png"（带 word/ 前缀）被解析为 word/word/media/…
        # ——Word 打开修复对话框级缺陷（S6 复测经 verify_s6 rels 悬空检查暴露，
        # S5 基线产物即带此形态；引擎 M5 收尾复核的 normpath 归一掩盖了它）
        rel_target = self.media_dir[5:] if self.media_dir.startswith("word/") \
            else self.media_dir
        self.rels_xml = self.rels_xml.replace(
            "</Relationships>",
            '<Relationship Id="%s" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="%s"/></Relationships>'
            % (rid, rel_target + media_name))

    def register(self, src_path):
        # type: (str) -> Tuple[str, str]
        """磁盘图片文件登记入包（同移植源 register；Default 按需幂等补）"""
        if src_path in self._used:
            return self._used[src_path]
        with open(src_path, "rb") as f:
            data = f.read()
        ext = os.path.splitext(src_path)[1].lower() or ".png"
        rid = self.next_rid()
        media_name = "image%d%s" % (self._next_img, ext)
        self._next_img += 1
        self._append_rel(rid, media_name)
        self._ensure_media_default(ext)
        self._bytes[media_name] = data
        self._used[src_path] = (rid, media_name)
        return rid, media_name

    def _ensure_media_default(self, ext):
        # type: (str) -> None
        """Content_Types 补媒体扩展 Default（幂等；ext 含点；A-06/T-01-④）"""
        ct = MEDIA_CONTENT_TYPES.get(ext)
        if ct and ('Extension="%s"' % ext[1:]) not in self.content_types:
            self.content_types = self.content_types.replace(
                "</Types>",
                '<Default Extension="%s" ContentType="%s"/></Types>' % (ext[1:], ct))

    def drain_media(self):
        # type: () -> Dict[str, bytes]
        """返回 {落包部件全名: 字节}，由引擎并入输出包部件表"""
        out = {}
        for media_name, data in self._bytes.items():
            out[self.media_dir + media_name] = data
        return out


def add_part_relationship(registry, rel_type, target):
    # type: (MediaRegistry, str, str) -> str
    """document.xml.rels 追加部件关系（移植 export_docx.py:926），返回新 Id"""
    rid = registry.next_rid()
    registry.rels_xml = registry.rels_xml.replace(
        "</Relationships>",
        '<Relationship Id="%s" Type="%s" Target="%s"/></Relationships>'
        % (rid, rel_type, target))
    return rid


def ensure_content_type_override(registry, part_name, content_type):
    # type: (MediaRegistry, str, str) -> None
    """[Content_Types].xml 补部件 Override（幂等；移植 export_docx.py:936；
    PART_CONTENT_TYPES 提供 MSFT 扩展部件的 ContentType 兜底查询——T-01-④）"""
    if 'PartName="%s"' % part_name not in registry.content_types:
        registry.content_types = registry.content_types.replace(
            "</Types>",
            '<Override PartName="%s" ContentType="%s"/></Types>'
            % (part_name, content_type))


# ---------------------------------------------------------------------------
# 版心尺寸与图片（移植 export_docx.py:299/:348/:356/:368）
# ---------------------------------------------------------------------------

def scaled_emu(pixel_w, pixel_h, box_w, box_h):
    # type: (Optional[int], Optional[int], int, int) -> Tuple[int, int]
    """像素尺寸等比缩放入 EMU 框（移植 export_docx.py:348；FR-24 版心/格宽等比）"""
    if not pixel_w or not pixel_h:
        return (box_w, int(box_w * 3 / 4))
    scale = min(box_w / float(pixel_w), box_h / float(pixel_h))
    return (int(pixel_w * scale), int(pixel_h * scale))


def body_width_emu(sectpr):
    # type: (str) -> int
    """从 sectPr 解析正文可用宽度（页宽-左右边距，EMU）（移植 export_docx.py:356）"""
    try:
        w = int(re.search(r'<w:pgSz w:w="(\d+)"', sectpr).group(1))
        mar = re.search(r"<w:pgMar[^>]*/>", sectpr).group(0)
        left = int(re.search(r'w:left="(\d+)"', mar).group(1))
        right = int(re.search(r'w:right="(\d+)"', mar).group(1))
        return (w - left - right) * TWIP_TO_EMU
    except Exception:
        return DEFAULT_BODY_WIDTH_EMU


def sect_body_height_twip(sectpr):
    # type: (str) -> int
    """版心可用高（页高-上下边距，twip）（移植 export_docx.py:368）"""
    try:
        h = int(re.search(r'<w:pgSz[^>]*w:h="(\d+)"', sectpr).group(1))
        mar = re.search(r"<w:pgMar[^>]*/>", sectpr).group(0)
        top = int(re.search(r'w:top="(\d+)"', mar).group(1))
        bottom = int(re.search(r'w:bottom="(\d+)"', mar).group(1))
        return h - top - bottom
    except Exception:
        return DEFAULT_BODY_HEIGHT_TWIP


def picture_run_xml(rid, doc_pr_id, cx, cy):
    # type: (str, int, int, int) -> str
    """内联图片 run XML（移植 export_docx.py:299 image_paragraph 的 run 形态；
    a/pic 命名空间在元素级内联声明——不依赖根标签声明，任何宿主文档可用）"""
    return (
        '<w:r><w:drawing>'
        '<wp:inline distT="0" distB="0" distL="114300" distR="114300">'
        '<wp:extent cx="%d" cy="%d"/>'
        '<wp:effectExtent l="0" t="0" r="0" b="0"/>'
        '<wp:docPr id="%d" name="Picture %d"/>'
        '<wp:cNvGraphicFramePr>'
        '<a:graphicFrameLocks xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" noChangeAspect="1"/>'
        '</wp:cNvGraphicFramePr>'
        '<a:graphic xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main">'
        '<a:graphicData uri="http://schemas.openxmlformats.org/drawingml/2006/picture">'
        '<pic:pic xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture">'
        '<pic:nvPicPr><pic:cNvPr id="%d" name="Picture %d"/>'
        '<pic:cNvPicPr><a:picLocks noChangeAspect="1"/></pic:cNvPicPr></pic:nvPicPr>'
        '<pic:blipFill><a:blip xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" r:embed="%s"/>'
        '<a:stretch><a:fillRect/></a:stretch></pic:blipFill>'
        '<pic:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="%d" cy="%d"/></a:xfrm>'
        '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom></pic:spPr>'
        '</pic:pic></a:graphicData></a:graphic></wp:inline>'
        '</w:drawing></w:r>'
    ) % (cx, cy, doc_pr_id, doc_pr_id, doc_pr_id, doc_pr_id, rid, cx, cy)


def clear_highlight_xml(xml_text):
    # type: (str) -> str
    """清高亮（highlight_clear 白名单项）：移除全部 w:highlight 元素——自闭合
    与配对形态（<w:highlight …>…</w:highlight>）双兼容（CA-11：配对形态残留
    会致 FR-32"全篇 highlight=0"假失败）。run rPr 与段落标记 rPr 双处同型
    生效——T-22；仅删元素本身，rPr 其余子元素与结构原样。"""
    return re.sub(r"</?w:highlight(?:\s[^>]*)?>", "", xml_text)


def strip_bookmark(xml_text, name):
    # type: (str, str) -> str
    """剥离指定名的书签对（_GoBack 游标书签剥离，C3-02/H-11）：
    仅删 bookmarkStart/End 标签自身，包裹内容零改动。"""
    xml_text = re.sub(r'<w:bookmarkStart w:id="\d+" w:name="%s"\s*/>' % re.escape(name),
                      "", xml_text)
    # bookmarkEnd 无 name，需按 Start 的 id 对应删除——调用方走 strip_bookmark_by_id
    return xml_text


def strip_bookmark_by_id(xml_text, bm_id):
    # type: (str, str) -> str
    """按 w:id 删除 bookmarkStart/End 标签（与 strip_bookmark 配套）"""
    xml_text = re.sub(r'<w:bookmarkStart w:id="%s"[^>]*/>' % re.escape(bm_id),
                      "", xml_text)
    xml_text = re.sub(r'<w:bookmarkEnd w:id="%s"\s*/>' % re.escape(bm_id),
                      "", xml_text)
    return xml_text
