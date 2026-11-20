#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MxSim 报告导出 Word 生成脚本（纯标准库实现）
读取 JSON 报告上下文 + docx 模板包，产出真 OOXML .docx。
default（默认母版）：同构替换——保留模板中第一个"章节标题样式"段落之前的
全部内容（封面/设计参数表/摘要/目录）原样（20261103-R09 起：封面设计参数表
右列值清空、左列标签保留，见 clear_cover_table），自该段落起丢弃示例章节，
再逐视图页生成真实章节（标题=页名 + 图片）。
custom（用户模板，20261105-R18 起，重写取代 R17"全文+仅标题图片章节"）：
合并产物 = 用户模板内容（全部原样，在前）+ 母版形式（新页起追加在后）。
**合并基准包 = 用户模板**（document.xml/文末 sectPr/页眉页脚部件/rels/styles/
主题/fontTable 全部用户的，整篇归用户最后一节 → 自动继承用户页眉页脚）；
自母版包仅移植"骨架 + 所需样式/编号/媒体"：
  骨架 = 母版 body 中第一个章节标题样式段落（styleId="2"）之前的全部顶层
    元素（封面/设计参数表/摘要/目录，与 default 分支 head 推导同源；示例
    章节剔除），执行 clear_cover_table（R09 值清空——仅母版骨架，用户内容
    永不清）与封面标题"迈曦设计报告"→"设计报告"替换（R20：母版文件保持
    原文、去"迈曦"仅作用本骨架字符串，用户内容不经过），并剥离片段内嵌
    w:sectPr（保证整篇归用户末节）、
    w:bookmarkStart/w:bookmarkEnd（杜绝 id/名冲突）、脚注/尾注/批注引用
    （防悬空引用损坏文档）；骨架首元素另起一页（w:p 则 pPr 加
    pageBreakBefore，否则前置带 pageBreakBefore 的空段）。
  样式 = 骨架+生成章节引用的全部样式自母版 styles.xml 按 basedOn/link 等
    传递链整链复制，styleId 一律加前缀 MxTpl_ 重命名、w:name 加后缀防撞，
    回写片段引用后追加进用户 styles.xml（缺部件则创建+关系+Content_Types）；
  编号 = 母版 numbering.xml 中 numId=1（章节自动编号）及其 abstractNum 整链
    复制，id 重编号为用户包现存 max+1（用户包缺部件则创建；母版无编号时
    生成章节 numId=0 降级——Word 忽略编号，不损坏）；
  媒体 = 片段内 r:embed/r:id/r:link 指向的母版关系/媒体接力登记进用户包
    （MediaRegistry 模式，外部链接 TargetMode 保留）。
生成章节与 default 完全同一套代码路径（标题/宫格/页大图/图注/行高预算），
仅样式 id 用重命名后的母版 id、numId 用重编号后的值；分页=骨架首元素+骨架
"目录"标题段+每个生成章节标题三处（R19 起 custom 全部加 pageBreakBefore——
恢复 R17 形参机制，章节连续排版为用户实测否决的行为；default 不带）；
缩进隔离（R19）=骨架无 pStyle 段落与生成图片段落挂重命名后的母版 Normal
（MxTpl_1，防用户 Normal/docDefaults 默认缩进对无样式段落逐属性渗入）、
闭包样式 w:ind 显式化（无 w:ind 补全零；有 w:ind 无 firstLine/firstLineChars
且无 hanging 的补该两属性=0；母版 left/hanging 意图优先不覆盖；用户内容/
用户 styles.xml 原有条目/docDefaults 一律不碰）；docPr id 按合并后全文重算。
母版页眉页脚部件结构性零引入（不在移植清单）→ 输出包无母版页眉页脚。
不依赖 python-docx / lxml / Pillow，离线打包环境可用（同 export_stats.py 先例）。

用法: python export_docx.py <report_ctx.json> <output.docx> <template.docx> <custom|default> [master.docx]
      template.docx 由 C++ 侧保证为真实磁盘文件（自定义=用户所选路径，
      默认=临时目录释放的内置报告母版副本）；第 4 参数=模板类别旗标（C++ 按
      实际所用模板传入：自定义首导=custom、无效回退/失败重导=default；缺失
      或非法即 usage 退出，不做静默默认——防调用方漂移悄悄测错路径）；
      第 5 参数仅 kind=custom 时必需（=合并源母版路径，C++ 临时目录释放的
      内置报告母版副本；缺/坏 → stderr 报错 rc=1，C++ 侧回退流程自然兜底），
      kind=default 时忽略（不传或传空串均可，脚本不校验）。
"""

import json
import re
import sys
import os
import zipfile
from xml.sax.saxutils import escape

# 母版示例章节标题所用段落样式（解包 design/report-export/报告母版.docx 实测：
# styleId=2 -> styles.xml "heading 1"；母版另带编号 numId=1 自动出 "1. 2. ..."。
# default 同构替换与 custom 母版骨架（R18）均按该样式段落定界；生成章节标题
# （default 固定 id，custom 用重命名后的母版 id——见 STYLE_RENAME_PREFIX）
CHAPTER_STYLE_ID = "2"
CHAPTER_NUMID = "1"
# 图注段落样式（styleId=6 -> styles.xml "caption"）
CAPTION_STYLE_ID = "6"
# 1 twip = 635 EMU；正文宽度兜底值（A4 11906 - 左右边距 720x2 twip）
TWIP_TO_EMU = 635
DEFAULT_BODY_WIDTH_EMU = 6645910
# 版心高兜底值（母版实测 sectPr：A4 16838 - 上下边距 720x2 = 15398 twip；
# 实际以运行时 sectPr 解析为准，此处仅为无 sectPr 的异常模板兜底）
DEFAULT_BODY_HEIGHT_TWIP = 15398
# 单行文字高度的保守估计倍数（字体上伸/下伸+行隙实测约 1.15，取 1.3 留裕量，
# 保证单页预算不低估实际行高）
LINE_FACTOR = 1.3
# 勾选（每图窗一图）时的表格布局：列数=节级布局 gridCols（0=自动 3 列，
# 20261102-R07 报告导出跟随页面布局；预设最大 4 列），单列宽（twip）=
# 版心宽 // cols（母版版心 10466，3 列时 3488；R08 起列宽公式不变、
# 行高改为撑满版心——等比时代逐字节基线已作废）
FIG_ROW_COLS = 3
# 单元格左右内边距（twip，母版 tcMar 实测值）；单列内图片宽=列宽-2x 本值
# （3 列时 3488-216=3272，转 EMU 约 2077720，与改前常量一致）
FIG_CELL_MAR_TWIP = 108
# 行图高压缢单格下限（twip）：极多图预算算尽时保底，防零/负图片尺寸损坏文档
FIG_IMG_H_FLOOR_TWIP = 240
# 封面设计参数表（R09）值清空判定标签：保留区内某表格存在首列文本为
# 两者之一的行即认定为封面表（母版实测首列依次=业务职能/项目群 / 项目主线/
# 项目组/产品编号/平台/计划发布时间/报告日期，值列 2 个 w:t 示例值）；
# 自定义模板无该表时静默跳过
COVER_TABLE_LABELS = ("业务职能", "产品编号")

# 母版合并（custom，20261105-R18）样式移植重命名：styleId 一律加前缀防撞
# 用户包既有 id（母版数字 id 与用户冲突概率高），w:name 同步加后缀防样式
# 显示名重名（Word 按名定位内置样式，重名会串样式）
STYLE_RENAME_PREFIX = "MxTpl_"
STYLE_NAME_SUFFIX = "_MxTpl"

# 片段媒体接力登记时补 Content_Types Default 的扩展名 → ContentType 映射
MEDIA_CONTENT_TYPES = {
    ".png": "image/png", ".jpeg": "image/jpeg", ".jpg": "image/jpeg",
    ".gif": "image/gif", ".bmp": "image/bmp", ".tif": "image/tiff",
    ".tiff": "image/tiff", ".emf": "image/x-emf", ".wmf": "image/x-wmf",
}


def read_package(path):
    """读 zip 包全部非目录条目为 {name: bytes}"""
    pkg = {}
    with zipfile.ZipFile(path, "r") as z:
        for info in z.infolist():
            if info.filename.endswith("/"):
                continue
            pkg[info.filename] = z.read(info.filename)
    return pkg


def png_pixel_size(path):
    """读 PNG IHDR 取像素宽高（截图统一 PNG）；非 PNG/解析失败返回 None"""
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


# ---------------------------------------------------------------------------
# document.xml 顶层结构解析（w:p/w:tbl/w:sdt/w:sectPr 的顶层片段定位）
# ---------------------------------------------------------------------------

_FIRST_TAG = re.compile(r"<(w:p|w:tbl|w:sdt|w:sectPr)\b[^>]*?(/?)>")


def top_level_children(body):
    """w:body 内顶层元素片段 [(tag, start, end)]（按文档顺序；
    嵌套同名标签按深度配对，正文里散落的书签等其余标签跳过）"""
    children = []
    i = 0
    n = len(body)
    while i < n:
        lt = body.find("<", i)
        if lt < 0:
            break
        m = _FIRST_TAG.match(body, lt)
        if not m:
            i = lt + 1
            continue
        tag = m.group(1)
        if m.group(2):   # 自闭合
            children.append((tag, lt, m.end()))
            i = m.end()
            continue
        pat = re.compile(r"<(/?)%s\b[^>]*?(/?)>" % tag)
        depth = 1        # 起始标签已被 match 消费，从 1 起配对
        end = n
        for mm in pat.finditer(body, m.end()):
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


# ---------------------------------------------------------------------------
# 生成段落/表格 XML（复用母版样式：章节标题 styleId=2 + 编号、图注 styleId=6）
# ---------------------------------------------------------------------------

def _child_spans(frag, tag):
    """frag 内顶层 tag 元素配对片段 [(start, end)]（与 top_level_children 同式
    深度配对，单元格内嵌套表的同名标签不误配；供行/单元格级定位复用）"""
    spans = []
    pat = re.compile(r"<(/?)%s\b[^>]*?(/?)>" % tag)
    i, n = 0, len(frag)
    while i < n:
        m = pat.search(frag, i)
        if not m:
            break
        if m.group(1):      # 孤立闭合标签（不应出现，防御性跳过）
            i = m.end()
            continue
        if m.group(2):      # 自闭合
            spans.append((m.start(), m.end()))
            i = m.end()
            continue
        depth = 1
        end = n
        for mm in pat.finditer(frag, m.end()):
            if mm.group(2):
                continue
            depth += 1 if mm.group(1) == "" else -1
            if depth == 0:
                end = mm.end()
                break
        spans.append((m.start(), end))
        i = end
    return spans


def _fragment_text(frag):
    """片段内全部 w:t 文本拼接（无则空串）"""
    return "".join(re.findall(r"<w:t[^>]*>([^<]*)</w:t>", frag))


def clear_cover_table(head):
    """封面设计参数表值清空（R09）：定位保留区内封面表，清空每行非首列
    单元格的全部 w:t 文本——左列标签保留、值列留白供用户导出后自填；
    仅清 w:t 文本节点，w:p/w:r/rPr 结构与格式原样，单元格高度
    （trHeight hRule=exact）与边框不受影响；"报告日期"值本为空，幂等。
    判定：表格中存在首列文本为 COVER_TABLE_LABELS 之一的行（避免误伤
    摘要表/正文其他表格）；找不到时原样返回——自定义模板无该表静默跳过。
    调用时机：head 定界（sectPr 保留/示例章节丢弃）之后，仅作用保留区，
    与生成章节无交集。返回新 head。"""
    for tag, a, b in top_level_children(head):
        if tag != "w:tbl":
            continue
        tbl = head[a:b]
        rows = _child_spans(tbl, "w:tr")
        if not rows:
            continue
        hit = False
        for ra, rb in rows:
            cells = _child_spans(tbl[ra:rb], "w:tc")
            if cells and _fragment_text(
                    tbl[ra + cells[0][0]:ra + cells[0][1]]).strip() in COVER_TABLE_LABELS:
                hit = True
                break
        if not hit:
            continue
        rebuilt, pos = [], 0
        for ra, rb in rows:
            row = tbl[ra:rb]
            cells = _child_spans(row, "w:tc")
            piece, cpos = [], 0
            for ci, (ca, cb) in enumerate(cells):
                if ci == 0:      # 首列标签单元格整段保留
                    piece.append(row[cpos:cb])
                else:            # 值单元格：w:t 置空，标签外结构/格式原样
                    piece.append(row[cpos:ca]
                                 + re.sub(r"(<w:t\b[^>]*>)[^<]*(</w:t>)", r"\1\2",
                                          row[ca:cb]))
                cpos = cb
            piece.append(row[cpos:])
            rebuilt.append("".join(piece))
            pos = rb
        new_tbl = tbl[:rows[0][0]] + "".join(rebuilt) + tbl[pos:]
        return head[:a] + new_tbl + head[b:]
    return head


def heading_paragraph(text, style_id=CHAPTER_STYLE_ID, page_break=False,
                      num_id=CHAPTER_NUMID):
    """章节标题段落：复用母版章节标题样式与自动编号（同设计图01.2 "1. 页名"）；
    缺编号定义时 Word 忽略编号，仅按样式渲染，不损坏文档。
    style_id：custom（R18）传重命名后的母版章节标题 styleId（默认=母版固定
    id，default 路径调用不传、产出逐字节不变）。page_break：custom（R19 起）
    恒 True——每个生成章节标题另起一页（R18 曾废除由骨架独担分页，用户实测
    章节连续排版不合意，恢复 R17 形参机制；骨架首元素分页仍保留）；default
    False 逐字节不变。schema 序：pStyle 之后、numPr 之前。num_id：custom
    （R18）传重编号后的母版章节 numId（母版无 numbering 时为 0=无编号降级；
    default 不传=母版固定 1）"""
    return (
        '<w:p><w:pPr><w:pStyle w:val="%s"/>%s'
        '<w:numPr><w:ilvl w:val="0"/><w:numId w:val="%s"/></w:numPr>'
        '</w:pPr><w:r><w:t xml:space="preserve">%s</w:t></w:r></w:p>'
    ) % (style_id, '<w:pageBreakBefore/>' if page_break else '', num_id,
         escape(text))


def caption_paragraph(text, style_id=CAPTION_STYLE_ID):
    """图注段落：母版图注样式（caption）+ 居中，文本为宿主组装的完整图注；
    style_id：custom（R18）传重命名后的母版图注 styleId（默认=母版固定 id）"""
    return (
        '<w:p><w:pPr><w:pStyle w:val="%s"/><w:jc w:val="center"/></w:pPr>'
        '<w:r><w:t xml:space="preserve">%s</w:t></w:r></w:p>'
    ) % (style_id, escape(text))


def image_paragraph(rid, doc_pr_id, cx, cy, style_id=None):
    """居中图片段落（wp:inline 内嵌，克隆母版图片段结构）。显示尺寸即
    cx/cy（Word 按显式 extent 渲染）——20261102-R08 拉伸充满直接写格子
    宽高；noChangeAspect 仅锁用户拖拽手柄，不影响按 cx/cy 渲染。
    style_id：custom（R19 缩进隔离 a）传重命名后的母版 Normal id——图片
    段落无 pStyle 会继承用户 Normal/docDefaults 默认缩进/字体而渗入，挂
    母版 Normal 隔离（jc=center 直排属性保留、优先于样式）；default 不传
    =无 pStyle（逐字节不变）"""
    return (
        '<w:p><w:pPr>%s<w:jc w:val="center"/></w:pPr>'
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
        '<pic:blipFill><a:blip r:embed="%s"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill>'
        '<pic:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="%d" cy="%d"/></a:xfrm>'
        '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom></pic:spPr>'
        '</pic:pic></a:graphicData></a:graphic></wp:inline>'
        '</w:drawing></w:r></w:p>'
    ) % ('<w:pStyle w:val="%s"/>' % style_id if style_id else '',
         cx, cy, doc_pr_id, doc_pr_id, doc_pr_id, doc_pr_id, rid, cx, cy)


def figure_table_cell(img_xml, caption_xml, col_w_twip):
    """宫格表格单元格：图片 + 图注（克隆母版 Figure7~12 的"图上图注"排布）"""
    return (
        '<w:tc><w:tcPr><w:tcW w:w="%d" w:type="dxa"/><w:jc w:val="center"/>'
        '<w:tcMar><w:top w:w="0" w:type="dxa"/><w:left w:w="108" w:type="dxa"/>'
        '<w:bottom w:w="0" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tcMar>'
        '</w:tcPr>%s%s</w:tc>'
    ) % (col_w_twip, img_xml, caption_xml)


def figure_table(rows_cells, cols, col_w_twip):
    """Figure 宫格表（每行 cols 格，整表居中；无表样样式依赖）"""
    grid = "".join('<w:gridCol w:w="%d"/>' % col_w_twip for _ in range(cols))
    trs = []
    for cells in rows_cells:
        trs.append("<w:tr>%s</w:tr>" % "".join(cells))
    return (
        '<w:tbl><w:tblPr><w:tblW w:w="0" w:type="auto"/><w:jc w:val="center"/>'
        '<w:tblLayout w:type="fixed"/>'
        '<w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:left w:w="108" w:type="dxa"/>'
        '<w:bottom w:w="0" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar>'
        '</w:tblPr><w:tblGrid>%s</w:tblGrid>%s</w:tbl><w:p/>'
    ) % (grid, "".join(trs))


def scaled_emu(pixel_w, pixel_h, box_w, box_h):
    """像素尺寸等比缩放入 EMU 框；尺寸未知时按框宽 4:3 兜底"""
    if not pixel_w or not pixel_h:
        return (box_w, int(box_w * 3 / 4))
    scale = min(box_w / float(pixel_w), box_h / float(pixel_h))
    return (int(pixel_w * scale), int(pixel_h * scale))


def body_width_emu(sectpr):
    """从文末 sectPr 解析正文可用宽度（页宽 - 左右边距，转 EMU）"""
    try:
        w = int(re.search(r'<w:pgSz w:w="(\d+)"', sectpr).group(1))
        mar = re.search(r"<w:pgMar[^>]*/>", sectpr).group(0)
        left = int(re.search(r'w:left="(\d+)"', mar).group(1))
        right = int(re.search(r'w:right="(\d+)"', mar).group(1))
        return (w - left - right) * TWIP_TO_EMU
    except Exception:
        return DEFAULT_BODY_WIDTH_EMU


def sect_body_height_twip(sectpr):
    """从文末 sectPr 实测版心可用高（页高 - 上/下边距，twip）；
    解析失败用母版实测值兜底"""
    try:
        h = int(re.search(r'<w:pgSz[^>]*w:h="(\d+)"', sectpr).group(1))
        mar = re.search(r"<w:pgMar[^>]*/>", sectpr).group(0)
        top = int(re.search(r'w:top="(\d+)"', mar).group(1))
        bottom = int(re.search(r'w:bottom="(\d+)"', mar).group(1))
        return h - top - bottom
    except Exception:
        return DEFAULT_BODY_HEIGHT_TWIP


def sect_grid_pitch_twip(sectpr):
    """docGrid 行网格间距（type="lines" 时行文字按网格行距对齐，母版实测
    312 twip）；非行网格返回 0（行高估算不做网格取整）"""
    m = re.search(r"<w:docGrid[^>]*/>", sectpr)
    if not m or 'w:type="lines"' not in m.group(0):
        return 0
    pm = re.search(r'w:linePitch="(\d+)"', m.group(0))
    return int(pm.group(1)) if pm else 0


# ---------------------------------------------------------------------------
# 母版合并（custom 分支，20261105-R18）：骨架提取/引用剥离/样式编号媒体移植
# ---------------------------------------------------------------------------

def _first_chapter_split(body, children):
    """第一个章节标题样式段落（顶层 w:p）在 body 中的起点；无则 None。
    default 分支 head 推导与母版骨架定界（R18）同源共用"""
    for tag, a, b in children:
        if tag == "w:p" and re.search(r'<w:pStyle w:val="%s"/>' % CHAPTER_STYLE_ID, body[a:b]):
            return a
    return None


def _strip_fragment_refs(frag):
    """剥离骨架片段中的悬空/冲突引用（宁降级不损坏，R18）：
    - 内嵌 w:sectPr（含 headerReference/footerReference 的分节符）——保证整篇
      归用户末节、母版页眉页脚零引入；
    - w:bookmarkStart/w:bookmarkEnd——杜绝与用户包书签 id/名冲突（TOC 锚点
      本就悬空，静态 TOC 既有取舍）；
    - 脚注/尾注/批注引用——母版 notes/comments 部件不移植，防悬空引用损坏文档"""
    frag = re.sub(r"<w:sectPr\b[^>]*>.*?</w:sectPr>", "", frag, flags=re.S)
    frag = re.sub(r"<w:sectPr\b[^>]*/>", "", frag)
    frag = re.sub(r"<w:bookmark(?:Start|End)\b[^>]*/>", "", frag)
    for _tag in ("footnoteReference", "endnoteReference", "commentReference"):
        frag = re.sub(r"<w:%s\b[^>]*/>" % _tag, "", frag)
    return frag


def _renum_numids(xml, num_map):
    """xml 内全部 <w:numId w:val="N"/> 重编号：num_map 命中→映射值，否则 0
    （0=无编号，Word 忽略——未移植的编号引用宁降级不损坏）"""
    return re.sub(r'(<w:numId w:val=")(\d+)(")',
                  lambda m: "%s%s%s" % (m.group(1), num_map.get(m.group(2), "0"),
                                        m.group(3)),
                  xml)


# ---------------------------------------------------------------------------
# 缩进隔离（custom 分支，20261106-R19）：用户模板 Normal/docDefaults 的缩进
# 属性按 OOXML 逐属性级联，会渗入母版形式中"无 pStyle 的段落"与"未显式声明
# w:ind 的被移植样式"（用户实测：中文模板常见 docDefaults 首行缩进 2 字符）。
# 对策全部作用在母版移植物一侧（骨架段落挂母版 Normal、闭包样式 w:ind 显式
# 化），用户内容/用户 styles.xml 原有条目/docDefaults 一律不经过
# ---------------------------------------------------------------------------

# 全零 w:ind（显式声明即隔离：left/leftChars/firstLine/firstLineChars 逐属性
# 覆盖用户默认，母版自身版式即"无缩进"）
IND_ZERO = '<w:ind w:left="0" w:leftChars="0" w:firstLine="0" w:firstLineChars="0"/>'
# CT_PPr 中按 schema 序排在 w:ind 之后的子元素：w:ind 插入点=首个命中者之前
# （合法 pPr 的文本序=schema 序，按序扫描首个命中即正确插入点；\b 防
# rPrChange 等更长标签误配）
_PPR_AFTER_IND = ("w:contextualSpacing", "w:mirrorIndents", "w:suppressOverlap",
                  "w:jc", "w:textDirection", "w:textAlignment", "w:textboxTightWrap",
                  "w:outlineLvl", "w:divId", "w:cnfStyle", "w:rPr", "w:sectPr",
                  "w:pPrChange")


def _explicit_style_ind(block):
    """闭包样式缩进显式化（R19 缩进隔离 b/c，仅作用于 MxTpl_ 前缀移植块）：
    - pPr 无 w:ind → 补全零 IND_ZERO（按 CT_PPr 序插到首个"ind 之后"子元素
      前；无 pPr 按 CT_Style 序（rPr/tblPr/trPr/tblStylePr 之前，均无则
      </w:style> 收尾前）构造）；
    - 已有 w:ind 且无 firstLine/firstLineChars 且无 hanging → 原属性原值
      保留、仅追加 firstLine="0" firstLineChars="0"（母版 left/right 等意图
      优先，只堵 docDefaults 首行缩进的属性级联渗入）；
    - 已有 hanging（或已声明首行）的原样不动（hanging 语义优先，母版 toc
      样式即此形态）"""
    pm = re.search(r"<w:pPr>.*?</w:pPr>", block, re.S)
    if pm:
        ppr = pm.group(0)
        ind = re.search(r"<w:ind\b[^>]*/>", ppr)
        if ind:
            cur = ind.group(0)
            if ('w:firstLine="' in cur or 'w:firstLineChars="' in cur
                    or 'w:hanging="' in cur):
                return block
            return block.replace(cur, cur[:-2] + ' w:firstLine="0" w:firstLineChars="0"/>',
                                 1)
        at = len(ppr) - len("</w:pPr>")
        for tag in _PPR_AFTER_IND:
            m = re.search(r"<%s\b" % tag, ppr)
            if m:
                at = m.start()
                break
        return block.replace(ppr, ppr[:at] + IND_ZERO + ppr[at:], 1)
    at = block.rfind("</w:style>")
    for tag in ("w:rPr", "w:tblPr", "w:trPr", "w:tblStylePr"):
        m = re.search(r"<%s\b" % tag, block)
        if m:
            at = min(at, m.start())
    return block[:at] + "<w:pPr>" + IND_ZERO + "</w:pPr>" + block[at:]


def _master_normal_style_id(master_styles_xml):
    """母版 Normal styleId（R19 缩进隔离 a 的挂靠目标，随闭包重命名为
    MxTpl_ 前缀）：styles.xml 中 w:type="paragraph" 且 w:default="1" 的样式；
    属性序不定，自样式块开标签双属性解析；取不到回退 "1"（母版实测值）"""
    for m in re.finditer(r"<w:style\b([^>]*)>", master_styles_xml):
        attrs = m.group(1)
        if 'w:type="paragraph"' in attrs and 'w:default="1"' in attrs:
            sid = re.search(r'w:styleId="([^"]+)"', attrs)
            if sid:
                return sid.group(1)
    return "1"


def _para_inject_pstyle(p, style_id):
    """单个 w:p 片段：无 pStyle 时 pPr 首位注 <w:pStyle w:val=.../>（CT_PPr
    schema 序 pStyle 为首子元素；无 pPr 构造插入；自闭合空段展开为常规形态，
    丢失的仅 w14:paraId/w:rsidR 等可忽略元数据属性）。已有 pStyle 原样返回"""
    open_m = re.match(r"<w:p\b[^>]*>", p)
    if not open_m:
        return p
    if open_m.group(0).endswith("/>"):
        return '<w:p><w:pPr><w:pStyle w:val="%s"/></w:pPr></w:p>' % style_id
    rest = p[open_m.end():]
    pm = re.match(r"<w:pPr>.*?</w:pPr>", rest, re.S)
    if pm:
        if "<w:pStyle" in pm.group(0):
            return p
        at = open_m.end() + len("<w:pPr>")
        return p[:at] + '<w:pStyle w:val="%s"/>' % style_id + p[at:]
    return (open_m.group(0) + '<w:pPr><w:pStyle w:val="%s"/></w:pPr>' % style_id
            + rest)


def _inject_skeleton_pstyle(frag, style_id):
    """骨架片段内全部无 pStyle 的 w:p（含表格/TOC 内容控件嵌套段落，R19
    缩进隔离 a）注 pStyle=母版 Normal——传入母版原始 styleId，随骨架引用
    回写统一改写为 MxTpl_ 前缀（种子集合多一个已存在 id，闭包不变）。仅
    作用骨架（母版形式），用户内容不经过本函数。表格/sdt 嵌套逐层下探，
    每趟只重写一个命中容器直至不动点；段内文本框嵌套段落不下探（母版骨架
    无此形态，防御边界）"""
    out, pos = [], 0
    for a, b in _child_spans(frag, "w:p"):
        out.append(frag[pos:a] + _para_inject_pstyle(frag[a:b], style_id))
        pos = b
    if out:
        out.append(frag[pos:])
        frag = "".join(out)
    for ctag in ("w:tbl", "w:sdt"):
        open_pat = re.compile(r"<%s\b[^>]*>" % ctag)
        close_pat = "</%s>" % ctag
        while True:
            hit = None
            for a, b in _child_spans(frag, ctag):
                blk = frag[a:b]
                op = open_pat.match(blk)
                cl = blk.rfind(close_pat)
                if not op or cl < 0:
                    continue
                inner = _inject_skeleton_pstyle(blk[op.end():cl], style_id)
                if inner != blk[op.end():cl]:
                    hit = (a, b, blk, op.end(), cl, inner)
                    break
            if hit is None:
                break
            a, b, blk, oe, cl, inner = hit
            frag = frag[:a] + blk[:oe] + inner + blk[cl:] + frag[b:]
    return frag


def _inject_toc_page_break(frag):
    """目录换页（R19）：骨架内拼接文本（strip 后）恰为"目录"的首个 w:p，
    pPr 注 <w:pageBreakBefore/>（schema 序由 _para_with_page_break 保证：
    pStyle/keepNext/keepLines 之后、spacing/ind 之前）。母版目录标题在 TOC
    内容控件（w:sdt）内而非 body 顶层，表格/sdt 嵌套逐层下探；找不到降级
    原样返回（不报错，退化母版容错）"""
    for a, b in _child_spans(frag, "w:p"):
        p = frag[a:b]
        if _fragment_text(p).strip() == "目录":
            return frag[:a] + _para_with_page_break(p) + frag[b:]
    for ctag in ("w:tbl", "w:sdt"):
        open_pat = re.compile(r"<%s\b[^>]*>" % ctag)
        close_pat = "</%s>" % ctag
        for a, b in _child_spans(frag, ctag):
            blk = frag[a:b]
            op = open_pat.match(blk)
            cl = blk.rfind(close_pat)
            if not op or cl < 0:
                continue
            inner = _inject_toc_page_break(blk[op.end():cl])
            if inner != blk[op.end():cl]:
                return frag[:a] + blk[:op.end()] + inner + blk[cl:] + frag[b:]
    return frag


def _merge_styles(master_styles_xml, seed_ids):
    """自母版 styles.xml 收集 seed_ids 及 basedOn/link 等传递链闭包整链
    （含样式块内 rStyle/tblStyle/pStyle 二级引用），styleId 加前缀、w:name
    加后缀重命名（防撞用户包既有 id/名），块内链上引用同步回写；每块经
    _explicit_style_ind 缩进显式化（R19 缩进隔离 b/c）。
    返回 (rename_map, 重命名+缩进显式化后的样式块列表)；引用缺失的 seed
    降级跳过（Word 忽略缺失样式引用，不损坏）。"""
    index = {}
    for m in re.finditer(r'<w:style\b[^>]*w:styleId="([^"]+)"[^>]*>.*?</w:style>',
                         master_styles_xml, re.S):
        index[m.group(1)] = m.group(0)
    closure = {}
    pending = set(seed_ids)
    while pending:
        sid = pending.pop()
        if sid in closure or sid not in index:
            continue
        block = index[sid]
        closure[sid] = block
        pending.update(re.findall(
            r'<w:(?:basedOn|link|next|rStyle|tblStyle|pStyle) w:val="([^"]+)"/>', block))
    rename = {sid: STYLE_RENAME_PREFIX + sid for sid in closure}
    renamed = []
    for sid, block in closure.items():
        block = re.sub(r'(<w:style\b[^>]*?w:styleId=")%s(")' % re.escape(sid),
                       lambda m: m.group(1) + rename[sid] + m.group(2),
                       block, count=1)
        block = re.sub(r'(<w:name w:val=")([^"]+)(")',
                       lambda m: m.group(1) + m.group(2) + STYLE_NAME_SUFFIX + m.group(3),
                       block, count=1)
        block = re.sub(r'(<w:(?:basedOn|link|next|rStyle|tblStyle|pStyle) w:val=")([^"]+)(")',
                       lambda m: "%s%s%s" % (m.group(1), rename.get(m.group(2), m.group(2)),
                                             m.group(3)),
                       block)
        renamed.append(_explicit_style_ind(block))
    return rename, renamed


def _relay_fragment_media(frag, master_pkg, registry):
    """骨架片段 r:embed/r:id/r:link 指向的母版关系/媒体接力登记进用户包
    （R18）：image 内部关系→母版 media 字节复制续名（MediaRegistry 模式）；
    TargetMode=External→关系原样重建（Target/TargetMode 保留，新 rId）；
    其余内部引用（母版部件不移植）降级剥离属性。返回重写后的片段。"""
    rels = {}
    mrels = master_pkg.get("word/_rels/document.xml.rels")
    if mrels is not None:
        for m in re.finditer(r'<Relationship\b([^>]*)/>', mrels.decode("utf-8")):
            rid_m = re.search(r'\bId="([^"]+)"', m.group(1))
            if rid_m:
                rels[rid_m.group(1)] = m.group(1)

    def _rewrite(m):
        attr, rid = m.group(1), m.group(2)
        info = rels.get(rid)
        if info is None:
            return ""      # 悬空引用：剥离属性
        external = 'TargetMode="External"' in info
        is_image = 'officeDocument/2006/relationships/image"' in info
        if external:
            new_rid = registry.next_rid()
            new_rel = re.sub(r'\bId="[^"]+"', 'Id="%s"' % new_rid,
                             "<Relationship%s/>" % info)
            registry.rels_xml = registry.rels_xml.replace(
                "</Relationships>", new_rel + "</Relationships>")
            return '%s="%s"' % (attr, new_rid)
        if is_image:
            target = re.search(r'\bTarget="([^"]+)"', info)
            part = target.group(1).lstrip("/")
            if not part.startswith("word/"):
                part = "word/" + part
            data = master_pkg.get(part)
            if data is None:
                return ""   # 媒体部件缺失：降级剥离
            new_rid, _name = registry.register_bytes(data, os.path.splitext(part)[1].lower())
            return '%s="%s"' % (attr, new_rid)
        return ""          # 其余内部引用（header/footer 等部件不移植）：剥离

    return re.sub(r'\b(r:embed|r:id|r:link)="([^"]+)"', _rewrite, frag)


def _merge_numbering(master_pkg, user_pkg, registry):
    """母版章节编号链移植（R18）：numId=CHAPTER_NUMID 及其 abstractNum 整链
    复制进用户包，id 重编号为用户包现存 max+1（abstractNumId/numId 分别；
    abstractNum 插在既有 num 之前，schema 序）；用户包缺 numbering.xml 则创建
    部件+关系+Content_Types；母版无 numbering/缺链时返回空 map（章节 numId=0
    降级，Word 忽略编号不损坏）。返回 num_map {母版 numId: 新 numId(str)}"""
    mnum = master_pkg.get("word/numbering.xml")
    if mnum is None:
        return {}
    mnum_xml = mnum.decode("utf-8")
    num_m = re.search(r'<w:num w:numId="%s"[^>]*>.*?</w:num>' % CHAPTER_NUMID,
                      mnum_xml, re.S)
    abs_ref = (re.search(r'<w:abstractNumId w:val="(\d+)"/>', num_m.group(0))
               if num_m else None)
    abs_m = (re.search(r'<w:abstractNum w:abstractNumId="%s"[^>]*>.*?</w:abstractNum>'
                       % abs_ref.group(1), mnum_xml, re.S)
             if abs_ref else None)
    if not (num_m and abs_m):
        return {}
    num_blk, abs_blk = num_m.group(0), abs_m.group(0)
    unum_raw = user_pkg.get("word/numbering.xml")
    unum_xml = (unum_raw.decode("utf-8")
                if unum_raw is not None and b"<w:numbering" in unum_raw
                and b"</w:numbering>" in unum_raw else "")
    max_num = max([int(x) for x in re.findall(r'<w:num w:numId="(\d+)"', unum_xml)] + [0])
    max_abs = max([int(x) for x in re.findall(r'<w:abstractNum w:abstractNumId="(\d+)"',
                                              unum_xml)] + [0])
    new_num, new_abs = max_num + 1, max_abs + 1
    num_map = {CHAPTER_NUMID: str(new_num)}
    abs_blk = re.sub(r'(<w:abstractNum w:abstractNumId=")\d+(")',
                     r'\g<1>%d\2' % new_abs, abs_blk, count=1)
    num_blk = re.sub(r'(<w:num w:numId=")\d+(")', r'\g<1>%d\2' % new_num, num_blk, count=1)
    num_blk = re.sub(r'(<w:abstractNumId w:val=")\d+(")',
                     r'\g<1>%d\2' % new_abs, num_blk, count=1)
    if unum_xml:
        at = unum_xml.find("<w:num ")
        if at < 0:
            at = unum_xml.rfind("</w:numbering>")
        unum_xml = _add_missing_namespaces(unum_xml, mnum_xml)
        at = unum_xml.find("<w:num ")
        if at < 0:
            at = unum_xml.rfind("</w:numbering>")
        user_pkg["word/numbering.xml"] = (unum_xml[:at] + abs_blk + num_blk
                                          + unum_xml[at:]).encode("utf-8")
    else:
        # 用户包缺 numbering 部件：创建（部件+关系+Content_Types Override；
        # 根标签沿用母版——声明随移植块完整携带）
        user_pkg["word/numbering.xml"] = (
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            + _part_root_tag(mnum_xml, "w:numbering")
            + abs_blk + num_blk + '</w:numbering>').encode("utf-8")
        ensure_content_type_override(
            registry, "/word/numbering.xml",
            "application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml")
        add_part_relationship(
            registry,
            "http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering",
            "numbering.xml")
    return num_map


def _build_master_skeleton(master_pkg, user_pkg, registry):
    """custom 母版合并主流程（R18）：提取骨架（母版 body 中第一个章节标题
    样式段落之前的全部顶层元素，示例章节剔除）→ 引用剥离 → R09 封面清值
    （仅母版骨架，用户内容永不清）→ R19 缩进隔离（骨架无 pStyle 段落挂母版
    Normal + "目录"标题段注 pageBreakBefore）→ 样式闭包移植（前缀重命名+
    缩进显式化）→ 编号链移植（重编号）→ 片段媒体接力 → 片段引用回写。
    用户包部件就地更新（styles/numbering 创建或追加，registry 承载 rels/
    Content_Types/媒体字节）。
    返回 (骨架片段, 章节 styleId, 图注 styleId, 章节 numId, Normal styleId)——
    后四者均为重命名/重编号后的值（生成章节与图片段挂靠用）"""
    mdoc = master_pkg["word/document.xml"].decode("utf-8")
    mbody_m = re.search(r"<w:body>(.*)</w:body>", mdoc, re.S)
    if not mbody_m:
        raise ValueError("master document.xml has no body")
    mbody = mbody_m.group(1)
    split_at = _first_chapter_split(mbody, top_level_children(mbody))
    skeleton = mbody[:split_at] if split_at is not None else mbody
    skeleton = _strip_fragment_refs(skeleton)
    skeleton = clear_cover_table(skeleton)
    # 封面标题去"迈曦"仅限 custom 链路（20261106-R20，收窄 R19 母版文案
    # 变更）：母版文件已整体还原为原文"迈曦设计报告"（default 导出直接用
    # 还原后的母版，标题保持原文，default 路径不经过本函数），母版形式随
    # 用户期望不带"迈曦"，故在此对骨架字符串执行替换——"迈曦设计报告"在
    # 母版包中为单一 w:t，直接字符串替换即可；仅作用骨架（母版来源部分），
    # 用户内容（user 包 body）不经过本函数，用户模板含"迈曦设计报告"字样
    # 原样保留
    skeleton = skeleton.replace("迈曦设计报告", "设计报告")

    # R19 缩进隔离 a：骨架内无 pStyle 段落（含表格/TOC 内容控件嵌套）统一
    # 挂母版 Normal——注入母版原始 styleId，随后骨架引用回写统一改写为
    # MxTpl_ 前缀；注入先于闭包种子收集，Normal 必在闭包（否则引用回写
    # 保留原 id 会误挂用户同 id 样式，故以种子保证）
    mstyles = master_pkg.get("word/styles.xml")
    normal_id = "1"
    if mstyles is not None:
        normal_id = _master_normal_style_id(mstyles.decode("utf-8"))
    skeleton = _inject_skeleton_pstyle(skeleton, normal_id)
    # R19 目录换页：骨架文本恰为"目录"的段注 pageBreakBefore（找不到降级
    # 原样返回；与骨架首元素分页各司其职——封面页/目录页分离）
    skeleton = _inject_toc_page_break(skeleton)

    # 样式闭包种子 = 骨架引用 + 生成章节引用（章节标题/图注固定取母版 id）
    seed = set(re.findall(r'<w:(?:pStyle|rStyle|tblStyle) w:val="([^"]+)"', skeleton))
    seed.update((CHAPTER_STYLE_ID, CAPTION_STYLE_ID))
    rename, renamed_blocks = ({}, [])
    if mstyles is not None:
        rename, renamed_blocks = _merge_styles(mstyles.decode("utf-8"), seed)

    num_map = _merge_numbering(master_pkg, user_pkg, registry)
    if num_map:
        renamed_blocks = [_renum_numids(b, num_map) for b in renamed_blocks]

    # 回写用户包 styles.xml：存在则文末追加（命名空间声明补齐），缺部件则
    # 创建（根标签沿用母版，+关系+Content_Types）；无 </w:styles> 的异常部件
    # 降级跳过（样式引用 Word 忽略，不损坏）
    if renamed_blocks:
        ustyles_raw = user_pkg.get("word/styles.xml")
        mstyles_xml = mstyles.decode("utf-8")
        if ustyles_raw is not None:
            ustyles = _add_missing_namespaces(ustyles_raw.decode("utf-8"), mstyles_xml)
            at = ustyles.rfind("</w:styles>")
            if at >= 0:
                user_pkg["word/styles.xml"] = (
                    ustyles[:at] + "".join(renamed_blocks) + ustyles[at:]).encode("utf-8")
        else:
            user_pkg["word/styles.xml"] = (
                '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                + _part_root_tag(mstyles_xml, "w:styles")
                + "".join(renamed_blocks) + '</w:styles>').encode("utf-8")
            ensure_content_type_override(
                registry, "/word/styles.xml",
                "application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml")
            add_part_relationship(
                registry,
                "http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles",
                "styles.xml")

    # 片段媒体/关系接力登记，随后回写片段引用（样式 id 重命名+编号重编号）
    skeleton = _relay_fragment_media(skeleton, master_pkg, registry)
    if rename:
        skeleton = re.sub(r'(<w:(?:pStyle|rStyle|tblStyle) w:val=")([^"]+)(")',
                          lambda m: "%s%s%s" % (m.group(1), rename.get(m.group(2), m.group(2)),
                                                m.group(3)),
                          skeleton)
    if num_map:
        skeleton = _renum_numids(skeleton, num_map)
    return (skeleton,
            rename.get(CHAPTER_STYLE_ID, CHAPTER_STYLE_ID),
            rename.get(CAPTION_STYLE_ID, CAPTION_STYLE_ID),
            num_map.get(CHAPTER_NUMID, "0"),
            rename.get(normal_id, normal_id))


def _para_with_page_break(p_frag):
    """段落片段 pPr 加 <w:pageBreakBefore/>（schema 序：pStyle/keepNext/
    keepLines 之后）；无 pPr 构造插入，自闭合空段整体替换"""
    open_m = re.match(r"<w:p\b[^>]*>", p_frag)
    if not open_m or open_m.group(0).endswith("/>"):
        return '<w:p><w:pPr><w:pageBreakBefore/></w:pPr></w:p>'
    open_tag = open_m.group(0)
    rest = p_frag[len(open_tag):]
    pm = re.match(r"<w:pPr>", rest)
    if pm:
        lead = re.match(r'(?:<w:pStyle\b[^>]*/>|<w:keepNext\b[^>]*/>|<w:keepLines\b[^>]*/>)*',
                        rest[pm.end():])
        at = pm.end() + lead.end()
        return open_tag + rest[:at] + "<w:pageBreakBefore/>" + rest[at:]
    return open_tag + "<w:pPr><w:pageBreakBefore/></w:pPr>" + rest


def style_vmetrics_twip(styles_xml, style_id, sz_default_hpt=21):
    """解析段落样式的垂直度量 (before, after, line_twip)：单行高按
    1.3 倍字号 x line/240 倍距保守估计（上界，保证单页预算不低估）。
    简化：不追 basedOn 继承链，无显式 w:sz 时按 Normal 常见字号 21 半点
    （10.5pt，母版 Normal 实测值）兜底；lineRule 仅支持母版的 auto。"""
    m = re.search(r'<w:style [^>]*w:styleId="%s"[^>]*>.*?</w:style>' % style_id,
                  styles_xml, re.S)
    if not m:
        return (0, 0, int(sz_default_hpt * 10 * LINE_FACTOR))
    block = m.group(0)

    def num(pat, default):
        mm = re.search(pat, block)
        return int(mm.group(1)) if mm else default

    before = num(r'<w:spacing[^>]*w:before="(\d+)"', 0)
    after = num(r'<w:spacing[^>]*w:after="(\d+)"', 0)
    line = num(r'<w:spacing[^>]*w:line="(\d+)"', 240)
    sz = num(r'<w:sz w:val="(\d+)"/>', sz_default_hpt)
    single = int(sz * 10 * LINE_FACTOR)   # 半点字号 -> twip 单行高（sz/2 pt x 20 x 1.3）
    return (before, after, int(single * line / 240))


class MediaRegistry(object):
    """模板包媒体/关系登记：word/media/imageN 与 document.xml.rels 的 rId
    均接在模板现存最大编号之后，避免撞名；png Default 缺失时补登记。
    custom 母版合并（R18）复用同一登记器：register_bytes 把母版片段媒体
    字节接力入包，next_rid 供部件关系（styles/numbering）与外部链接续号"""

    def __init__(self, pkg, doc_xml, rels_xml, content_types):
        self.pkg = pkg
        self.rels_xml = rels_xml
        self.content_types = content_types
        if "</Relationships>" not in rels_xml:
            # 自闭合 rels 根（<Relationships .../>，部分极简模板如此）展开为
            # 常规形态，保证后续条目追加（replace "</Relationships>"）可写入
            m = re.search(r"(<Relationships\b[^>]*?)/>\s*$", rels_xml)
            if m:
                self.rels_xml = m.group(1) + "></Relationships>"
        max_rid = max((int(x) for x in re.findall(r'Id="rId(\d+)"', self.rels_xml)), default=0)
        max_img = 0
        for name in pkg:
            m = re.match(r"word/media/image(\d+)", name)
            if m:
                max_img = max(max_img, int(m.group(1)))
        self._next_rid = max_rid + 1
        self._next_img = max_img + 1
        self._used = {}   # src_path -> (rid, media_name)
        self._bytes = {}  # media_name -> data（母版片段媒体，源=母版 zip 条目）
        if 'Extension="png"' not in content_types:
            self.content_types = content_types.replace(
                "</Types>", '<Default Extension="png" ContentType="image/png"/></Types>')

    def next_rid(self):
        """续号分配下一个关系 Id（调用方自行写 rels 条目）"""
        rid = "rId%d" % self._next_rid
        self._next_rid += 1
        return rid

    def ensure_media_default(self, ext):
        """Content_Types 补媒体扩展 Default（幂等；ext 含点）"""
        ct = MEDIA_CONTENT_TYPES.get(ext)
        if ct and ('Extension="%s"' % ext[1:]) not in self.content_types:
            self.content_types = self.content_types.replace(
                "</Types>",
                '<Default Extension="%s" ContentType="%s"/></Types>' % (ext[1:], ct))

    def register(self, src_path):
        if src_path in self._used:
            return self._used[src_path]
        rid = self.next_rid()
        ext = os.path.splitext(src_path)[1].lower() or ".png"
        media_name = "image%d%s" % (self._next_img, ext)
        self._next_img += 1
        self.rels_xml = self.rels_xml.replace(
            "</Relationships>",
            '<Relationship Id="%s" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="media/%s"/></Relationships>'
            % (rid, media_name))
        self.ensure_media_default(ext)
        self._used[src_path] = (rid, media_name)
        return rid, media_name

    def register_bytes(self, data, ext=".png"):
        """母版片段媒体接力登记（R18）：媒体字节直接入包（源为母版 zip 条目
        而非磁盘文件），关系/媒体名与 register 同一套接力编号"""
        rid = self.next_rid()
        media_name = "image%d%s" % (self._next_img, ext)
        self._next_img += 1
        self.rels_xml = self.rels_xml.replace(
            "</Relationships>",
            '<Relationship Id="%s" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="media/%s"/></Relationships>'
            % (rid, media_name))
        self.ensure_media_default(ext)
        self._bytes[media_name] = data
        return rid, media_name


def add_part_relationship(registry, rel_type, target):
    """word/_rels/document.xml.rels 追加部件关系（styles/numbering 等部件
    创建时用，R18），返回新 Id"""
    rid = registry.next_rid()
    registry.rels_xml = registry.rels_xml.replace(
        "</Relationships>",
        '<Relationship Id="%s" Type="%s" Target="%s"/></Relationships>' % (rid, rel_type, target))
    return rid


def ensure_content_type_override(registry, part_name, content_type):
    """[Content_Types].xml 补部件 Override（幂等）"""
    if 'PartName="%s"' % part_name not in registry.content_types:
        registry.content_types = registry.content_types.replace(
            "</Types>",
            '<Override PartName="%s" ContentType="%s"/></Types>' % (part_name, content_type))


def _part_root_tag(xml_text, fallback_tag):
    """部件根标签（首个元素）；无法定位时返回最小声明根标签兜底"""
    m = re.search(r"<%s\b[^>]*>" % fallback_tag, xml_text)
    return m.group(0) if m else '<%s xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">' % fallback_tag


def _add_missing_namespaces(user_xml, master_xml):
    """把母版根标签的 xmlns 声明并入用户部件根标签（同前缀已声明跳过），
    防移植片段/样式块使用用户根未声明的命名空间（unbound prefix 损坏文档，
    R18；document/styles/numbering 移植统一走本助手）"""
    decl = re.match(r"\s*<\?xml[^>]*\?>", user_xml)
    head_end = decl.end() if decl else 0
    uroot = re.match(r"<([A-Za-z0-9:]+)\b[^>]*>", user_xml[head_end:])
    if not uroot:
        return user_xml
    mroot = re.search(r"<%s\b[^>]*>" % re.escape(uroot.group(1)), master_xml)
    if not mroot:
        return user_xml
    declared = set(re.findall(r'(xmlns:[A-Za-z0-9]+)=', uroot.group(0)))
    add_attrs = ['%s="%s"' % (m.group(1), m.group(2))
                 for m in re.finditer(r'(xmlns:[A-Za-z0-9]+)="([^"]+)"', mroot.group(0))
                 if m.group(1) not in declared]
    if not add_attrs:
        return user_xml
    cut = head_end + uroot.end() - 1     # 根标签收尾 '>' 前
    return user_xml[:cut] + " " + " ".join(add_attrs) + user_xml[cut:]


def build_docx(ctx, output_path, template_path, template_kind="default",
               master_path=None):
    pkg = read_package(template_path)
    for required in ("word/document.xml", "word/_rels/document.xml.rels", "[Content_Types].xml"):
        if required not in pkg:
            raise ValueError("not a valid docx package (missing %s): %s" % (required, template_path))
    doc = pkg["word/document.xml"].decode("utf-8")
    doc_m = re.search(r"<w:body>(.*)</w:body>", doc, re.S)
    if not doc_m:
        raise ValueError("template document.xml has no body")
    body = doc_m.group(1)

    children = top_level_children(body)

    # 文末顶层 sectPr（页面设置）原样保留
    tail_sectpr = ""
    for tag, a, b in children:
        if tag == "sectPr" or tag == "w:sectPr":
            tail_sectpr = body[a:b]

    # head 定界（模板类别分支，20261105-R18）：
    #   default——定位第一个章节标题样式段落：其前（封面/设计参数表/摘要/目录）
    #     原样保留，自该段落起丢弃示例章节。找不到时保留全部原文档、章节追加文末
    #   custom——跳过 split 搜索与示例章节丢弃：用户模板正文全文原样保留（含
    #     第一章标题后的自有内容），仅剥离文末 sectPr 供母版形式追加其前；
    #     同时读取合并源母版包（骨架/样式/编号/媒体的移植源，R18）
    custom = template_kind == "custom"
    if custom:
        head = body
        if tail_sectpr:
            sec_at = body.rfind(tail_sectpr)
            head, tail_sectpr = body[:sec_at], body[sec_at:]
        if not master_path:
            raise ValueError("custom export requires a master template path")
        master_pkg = read_package(master_path)
        for required in ("word/document.xml", "word/_rels/document.xml.rels"):
            if required not in master_pkg:
                raise ValueError("not a valid master docx package (missing %s): %s"
                                 % (required, master_path))
    else:
        split_at = _first_chapter_split(body, children)
        if split_at is None:
            head = body
            if tail_sectpr:
                sec_at = body.rfind(tail_sectpr)
                head, tail_sectpr = body[:sec_at], body[sec_at:]
        else:
            head = body[:split_at]

    # 封面设计参数表值清空（R09）：仅 default 生效（同构替换语义的一部分）；
    # custom 用户内容一字不改（R17：不清值）。default 下仅作用保留区 head——
    # 与上方 head 定界（sectPr 保留/示例章节丢弃）衔接，生成章节不含封面表
    # 无交集；模板无该表时 clear_cover_table 原样返回（静默跳过）
    if not custom:
        head = clear_cover_table(head)

    # 媒体/关系登记器（rels/内容类型文本后续整体写回包）
    registry = MediaRegistry(pkg, doc,
                             pkg["word/_rels/document.xml.rels"].decode("utf-8"),
                             pkg["[Content_Types].xml"].decode("utf-8"))
    doc_pr_id = max([int(x) for x in re.findall(r'<wp:docPr id="(\d+)"', doc)] + [0])

    # 母版合并（custom，20261105-R18）：骨架片段提取/引用剥离/封面清值 +
    # 样式闭包移植（前缀重命名，回写用户 styles.xml）+ 章节编号链移植（重
    # 编号）+ 片段媒体接力（registry 承载 rels/Content_Types/媒体字节）。
    # 生成章节样式·编号随之取移植值；default 固定母版 id（逐字节不变）。
    # 须在 styles_xml 读取之前执行 → 行高预算自动取合并后的用户 styles.xml
    chapter_num_id = CHAPTER_NUMID
    master_skel = ""
    tpl_normal_id = None
    if custom:
        master_skel, chapter_style_id, caption_style_id, chapter_num_id, tpl_normal_id = \
            _build_master_skeleton(master_pkg, pkg, registry)
    else:
        chapter_style_id = CHAPTER_STYLE_ID
        caption_style_id = CAPTION_STYLE_ID

    content_width = body_width_emu(tail_sectpr) if tail_sectpr else DEFAULT_BODY_WIDTH_EMU
    content_w_twip = content_width // TWIP_TO_EMU

    def fig_col_metrics(cols):
        """该节宫格列度量：列宽 twip=版心宽//cols，图片框宽 EMU=列宽−左右
        cellMar 108x2。cols=3 时与改前常量（3488/3272）同值（R07 布局跟随
        列数语义不变）"""
        col_w_twip = content_w_twip // cols
        box_w = (col_w_twip - FIG_CELL_MAR_TWIP * 2) * TWIP_TO_EMU
        return col_w_twip, box_w

    # 勾选态单页预算（20261102-R08 行高撑满版心）：版心高/行网格从模板
    # sectPr 实测，章节标题/图注行高从 styles.xml 解析保守估计（1.3 倍字号
    # 上界，行网格模式再向上取整）。行图高 = (版心高 − 标题高 − R×图注行高)
    # //R 再按行网格向下取整，R=⌈N/C⌉（C=节级布局列数，R07 语义不变），
    # 行高×R+图注+标题≈版心可用高——任意 N 单页且不留下半页空白（取代
    # R06/R07 的"4:3 高度上限+min 压缩"等比时代预算，旧基线已作废）；
    # 极多图预算算尽时 FIG_IMG_H_FLOOR_TWIP 保底，防零/负尺寸损坏文档
    body_h_twip = sect_body_height_twip(tail_sectpr) if tail_sectpr else DEFAULT_BODY_HEIGHT_TWIP
    grid_pitch = sect_grid_pitch_twip(tail_sectpr) if tail_sectpr else 0
    # custom（R18）：此处读到的是母版样式移植后的用户 styles.xml（上方合并已
    # 回写 pkg）→ 行高预算按重命名后的母版章节/图注样式度量解析
    styles_xml = pkg.get("word/styles.xml", b"").decode("utf-8", "ignore")
    hb, ha, hl = style_vmetrics_twip(styles_xml, chapter_style_id)
    cb, ca, cl = style_vmetrics_twip(styles_xml, caption_style_id)

    def grid_up(v):
        return -(-v // grid_pitch) * grid_pitch if grid_pitch else v

    def grid_down(v):
        return v - v % grid_pitch if grid_pitch else v

    heading_h_twip = hb + ha + grid_up(hl)      # 节标题段（keepNext 与表格同页）
    cap_h_twip = ca + grid_up(cl)               # 每格图注段

    def fig_row_box_h_emu(img_count, cols):
        """该节勾选态行图高（EMU）：行高=(版心高−标题高−R×图注行高)//R"""
        rows = (img_count + cols - 1) // cols
        budget = (body_h_twip - heading_h_twip - rows * cap_h_twip) // rows
        budget = max(grid_down(budget), FIG_IMG_H_FLOOR_TWIP)
        return budget * TWIP_TO_EMU

    # 逐节（视图页）生成章节：标题段落 + 图片内容
    #   勾选：每行 gridCols 图的表格排布（0=自动 3 列，R07 跟随页面布局；
    #   行高撑满版心，同节单页不跨页，见上方"勾选态单页预算"），图非等比
    #   拉伸充满所在格子（宽=列宽−216twip、高=行高，20261102-R08；单图
    #   N=1 独占整行版心——R08 §1"单视图页充满插入框"三格式一致语义），
    #   图注=JSON caption（宿主已组装完整图注），仍在图下
    #   不勾选（imagePerChart=false）：每节一张页大图（宿主拼合画布已同
    #   PPT 插入框比例），等比缩放到正文宽度（R08 未改此分支——Word 无
    #   固定插入框，竖向拉满版心会对 1.98 宽高比页图造成约 3 倍纵向变形）
    image_per_chart = bool(ctx.get("imagePerChart", False))
    generated = []
    for section in ctx.get("sections", []):
        title = section.get("title", "")
        imgs = [it for it in section.get("items", [])
                if it.get("type", 0) == 1 and os.path.exists(it.get("imagePath", ""))]
        if not imgs:
            continue
        # R19：custom 每个生成章节标题另起一页（pageBreakBefore，恢复 R17
        # 形参机制；骨架首元素分页保留不动）；default 不带（逐字节不变）；
        # 编号 numId 用移植重编号值
        generated.append(heading_paragraph(title, chapter_style_id,
                                           page_break=custom, num_id=chapter_num_id))
        if image_per_chart:
            cols = int(section.get("gridCols", 0) or 0) or FIG_ROW_COLS
            if len(imgs) == 1:
                cols = 1
            col_w_twip, fig_img_box_w = fig_col_metrics(cols)
            row_box_h = fig_row_box_h_emu(len(imgs), cols)
            cells_rows = []
            row = []
            for item in imgs:
                rid, _media = registry.register(item.get("imagePath", ""))
                doc_pr_id += 1
                img_xml = image_paragraph(rid, doc_pr_id, fig_img_box_w, row_box_h,
                                          style_id=tpl_normal_id)
                cap_xml = caption_paragraph(item.get("caption", ""), caption_style_id)
                row.append(figure_table_cell(img_xml, cap_xml, col_w_twip))
                if len(row) == cols:
                    cells_rows.append(row)
                    row = []
            if row:
                # 末行不满 cols 格补空格占位，保证表格列对齐
                while len(row) < cols:
                    row.append('<w:tc><w:tcPr><w:tcW w:w="%d" w:type="dxa"/></w:tcPr><w:p/></w:tc>'
                               % col_w_twip)
                cells_rows.append(row)
            generated.append(figure_table(cells_rows, cols, col_w_twip))
        else:
            for item in imgs:
                rid, _media = registry.register(item.get("imagePath", ""))
                pw, ph = scaled_emu(*(png_pixel_size(item.get("imagePath", "")) or (None, None)),
                                    content_width, int(content_width * 3))
                doc_pr_id += 1
                generated.append(image_paragraph(rid, doc_pr_id, pw, ph,
                                                 style_id=tpl_normal_id))

    # 母版形式追加（custom，R18）：骨架另起一页——首元素为 w:p 则其 pPr 加
    # pageBreakBefore，否则（表/sdt 开头或骨架为空的退化母版）前置一个带
    # pageBreakBefore 的空段落（骨架空且有生成章节时空段保证章节仍另起一页）。
    # R19 起分页共三处、各司其职：骨架首元素（封面页起）+骨架"目录"标题段
    # （目录页起，_inject_toc_page_break 已注入）+每个生成章节标题（上文
    # page_break=custom）；本段只管骨架首元素，后两处不经此路；
    # 骨架+生成章节并入用户最后一节（文末 sectPr 之前）→ 整篇继承用户页眉页脚
    if custom:
        sk_children = top_level_children(master_skel) if master_skel else []
        if sk_children and sk_children[0][0] == "w:p":
            sa, sb = sk_children[0][1], sk_children[0][2]
            master_skel = (master_skel[:sa]
                           + _para_with_page_break(master_skel[sa:sb])
                           + master_skel[sb:])
        elif master_skel or generated:
            master_skel = '<w:p><w:pPr><w:pageBreakBefore/></w:pPr></w:p>' + master_skel

    # 文末 sectPr 原样拼回（含 headerReference/footerReference）→ 母版形式与
    # 生成章节并入最后一节、继承其页眉页脚；header/footer 部件与关系整包随
    # pkg 原样保留（custom 模式页眉页脚即用户的——母版页眉页脚部件结构性
    # 零引入，default 为母版的），无需额外处理
    new_body = head + master_skel + "".join(generated) + (tail_sectpr or "")
    new_doc = doc[:doc_m.start(1)] + new_body + doc[doc_m.end(1):]

    if custom:
        # 命名空间声明补齐 + docPr id 按合并后全文重算（R18）：母版骨架片段
        # 可能使用用户文档根未声明的命名空间（w14:paraId 等）；用户原文与母版
        # 骨架的图形 docPr id 各起一头必撞，统一顺序重编保证全篇唯一
        new_doc = _add_missing_namespaces(new_doc,
                                          master_pkg["word/document.xml"].decode("utf-8"))
        _seq = [0]

        def _renumber_docpr(m):
            _seq[0] += 1
            return '<wp:docPr id="%d"' % _seq[0]

        new_doc = re.sub(r'<wp:docPr id="\d+"', _renumber_docpr, new_doc)

    pkg["word/document.xml"] = new_doc.encode("utf-8")
    pkg["word/_rels/document.xml.rels"] = registry.rels_xml.encode("utf-8")
    pkg["[Content_Types].xml"] = registry.content_types.encode("utf-8")

    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as z:
        for name, data in pkg.items():
            z.writestr(name, data)
        for src, (_rid, media_name) in registry._used.items():
            z.write(src, "word/media/" + media_name)
        for media_name, data in registry._bytes.items():
            z.writestr("word/media/" + media_name, data)


if __name__ == "__main__":
    # 第 4 参数=模板类别旗标（R17），必需且仅接受 custom/default——缺失/非法
    # 即 usage 退出，不做静默默认（防调用方漂移悄悄测错路径）；第 5 参数=
    # 合并源母版路径（R18），kind=custom 时必需且须可读（坏包在 build_docx
    # 读包时报错），kind=default 时忽略（不传/空串均可，不校验）
    if len(sys.argv) < 5:
        sys.stderr.write("usage: export_docx.py <ctx.json> <output.docx>"
                         " <template.docx> <custom|default> [master.docx]\n")
        sys.exit(1)
    try:
        with open(sys.argv[1], "r", encoding="utf-8") as f:
            ctx = json.load(f)
        if not ctx.get("templateMode"):
            raise ValueError("export_docx.py only handles templateMode exports")
        template = sys.argv[3]
        if not template or not os.path.exists(template):
            raise ValueError("template file missing: %r" % template)
        kind = sys.argv[4]
        if kind not in ("custom", "default"):
            raise ValueError("template kind must be 'custom' or 'default', got %r" % kind)
        master = sys.argv[5] if len(sys.argv) > 5 else ""
        if kind == "custom" and (not master or not os.path.exists(master)):
            raise ValueError("custom export requires a readable master template file: %r"
                             % master)
        build_docx(ctx, sys.argv[2], template, kind, master or None)
    except Exception as e:
        sys.stderr.write("docx export failed: %s\n" % e)
        sys.exit(1)
