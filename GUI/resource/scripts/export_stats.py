#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MxSim 曲线统计导出脚本（纯标准库实现）
读取 JSON 格式的统计上下文，直接生成 OOXML 格式的 .xlsx / .pptx 包。
不依赖 openpyxl / python-pptx / lxml，离线打包环境可用（与报告导出
export_pptx.py 同款机制，pptx 部分的包骨架/表格 XML 源自该脚本）。

用法: python export_stats.py <stats_ctx.json> <output.(xlsx|pptx)>

JSON 上下文结构（严格还原图面排版契约，宿主 mCurveStatisticsDlg 组装）:
{
    "title":  "50-1000Hz动刚度统计结果",  # 标题（仅 rows 布局；matrix 置 ""，无标题行）
    "layout": "rows",                     # "rows"=动刚度行式 / "matrix"=VTF/NTF 矩阵
    "corner": "工况",                      # 表左上角标（两布局首列）
    "groups": ["6001", "7016"],           # matrix 列头分组标签（对象号）；rows 为 []
    "spans":  [4, 5],                     # matrix 各组跨列数（≥1，与 groups 等长）；rows 为 []
    "rows":   [ ["Subcase 1 (1008_Z)", "(159-162)-(160@9599)", ...], ... ]
    # 数据行自第 2 行起与表格等宽；形如数字的单元格按数值写出，其余按文本
    "band_stats":                        # 第二张表"频段统计"（缺席/无 columns = 不加 Sheet/页）
    {
        "f_min": 20.0, "f_max": 200.0,
        #按组分段+行合并（2026-11-04 口径）：每组段宽=范围/组列数，列头逐组各自
        #生成（Σspans 个）；width 键已移除；同明细行键合并为一条频段行
        "columns": ["20-42.5", ..., "20-45", ...],       # Σspans 个"段起-段止"列头
        "rows":    [ ["Subcase 1 (Z)", 1, "", "", "", ...], ... ],
        #            行标签=明细行键；仅计数>0 写数字，0 与该行缺席组槽位一律空串=渲染留空
        "marks":   [ [true, false, ...], ... ]            # 与计数格同形：true=黄底标亮
    }
}
排版（严格对齐设计稿 01-05）：
  rows    第 1 行 = corner + 合并标题（B1 起跨数据全宽，宽 <3 列不合并），
          无列头行、无汇总列；
  matrix  第 1 行 = corner + 各组按 span 分组合并列头，无标题行；
  pptx    rows = 标题文本框 + 纯数据表（无表头行，firstRow 样式关闭）；
          matrix = 首行分组表头（gridSpan/hMerge 合并）+ 数据行，无标题文本框。
"""

import json
import os
import re
import sys
import zipfile
from xml.sax.saxutils import escape

# 形如数字（含科学计数/负号/小数）的单元格按数值写出，其余按文本
_NUM_RE = re.compile(r"^-?\d+(\.\d+)?([eE][+-]?\d+)?$")

# 单元格样式索引（styles.xml cellXfs）：0 默认 / 1 表格带框 / 2 表头与标题（粗体居中带框）
# / 3 频段统计标亮格（带框+黄底 FFFF00+居中，add-curve-stats-band-count）
# / 4 频段统计普通计数格（带框+居中；与 1 的差异仅对齐，明细表零感知）
STYLE_DEFAULT = 0
STYLE_BODY = 1
STYLE_HEAD = 2
STYLE_BAND_MARK = 3
STYLE_BAND_BODY = 4

# 频段统计标亮底色（用户需求：出现频段的那格改黄色）
BAND_MARK_COLOR = "FFFF00"

# 列宽（字符数）：首列工况 45（pch 子工况名收紧后仍较长），其余 24（容纳
# "(159-162)-(160@9599)" 型频段串；默认 8.43 会截断）；频段统计表列容
# "20-35" 型段头与 1-2 位计数，12 足够（2026-11-03 真机 vtf.xlsx 复盘收窄）
COL_WIDTH_FIRST = 45
COL_WIDTH_REST = 24
BAND_COL_WIDTH = 12

STYLES_XML = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
    '<fonts count="2">'
    '<font><sz val="11"/><name val="Calibri"/></font>'
    '<font><b/><sz val="11"/><name val="Calibri"/></font>'
    '</fonts>'
    '<fills count="3">'
    '<fill><patternFill patternType="none"/></fill>'
    '<fill><patternFill patternType="gray125"/></fill>'
    '<fill><patternFill patternType="solid"><fgColor rgb="FFFF00"/><bgColor indexed="64"/></patternFill></fill>'
    '</fills>'
    '<borders count="2">'
    '<border><left/><right/><top/><bottom/><diagonal/></border>'
    '<border>'
    '<left style="thin"><color auto="1"/></left>'
    '<right style="thin"><color auto="1"/></right>'
    '<top style="thin"><color auto="1"/></top>'
    '<bottom style="thin"><color auto="1"/></bottom>'
    '<diagonal/>'
    '</border>'
    '</borders>'
    '<cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>'
    '<cellXfs count="5">'
    '<xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/>'
    '<xf numFmtId="0" fontId="0" fillId="0" borderId="1" xfId="0" applyBorder="1"/>'
    '<xf numFmtId="0" fontId="1" fillId="0" borderId="1" xfId="0"'
    ' applyFont="1" applyBorder="1" applyAlignment="1">'
    '<alignment horizontal="center" vertical="center"/>'
    '</xf>'
    '<xf numFmtId="0" fontId="0" fillId="2" borderId="1" xfId="0"'
    ' applyFill="1" applyBorder="1" applyAlignment="1">'
    '<alignment horizontal="center"/>'
    '</xf>'
    '<xf numFmtId="0" fontId="0" fillId="0" borderId="1" xfId="0"'
    ' applyBorder="1" applyAlignment="1">'
    '<alignment horizontal="center"/>'
    '</xf>'
    '</cellXfs>'
    '</styleSheet>')

# 16:9 幻灯片（EMU，1 英寸 = 914400），与 export_pptx.py 一致
SLIDE_W = 12192000
SLIDE_H = 6858000


def is_number(text):
    return bool(_NUM_RE.match(text.strip())) if isinstance(text, str) else False


# ---------------------------------------------------------------------------
# xlsx 生成（最小 OOXML 电子表格包：内联字符串 + 数值单元格）
# ---------------------------------------------------------------------------

def xlsx_cell(ref, value, numeric, style=0):
    s = ' s="%d"' % style if style else ""
    if numeric:
        return '<c r="%s"%s><v>%s</v></c>' % (ref, s, value)
    return ('<c r="%s"%s t="inlineStr"><is><t xml:space="preserve">%s</t></is></c>'
            % (ref, s, escape(value)))


def col_letter(idx):
    """0 -> A, 25 -> Z, 26 -> AA"""
    name = ""
    idx += 1
    while idx:
        idx, rem = divmod(idx - 1, 26)
        name = chr(ord("A") + rem) + name
    return name


def build_sheet_xml(ctx):
    layout = ctx.get("layout", "rows")
    corner = ctx.get("corner", "工况")
    groups = ctx.get("groups", [])
    spans = ctx.get("spans", [])
    rows = ctx.get("rows", [])
    #标题仅行式布局有：矩阵按设计稿 04-05 无标题行
    title = ctx.get("title", "") if layout == "rows" else ""

    merges = []

    #首行：行式 = 角标 + 合并标题（B1 起跨数据全宽，宽 <3 列不合并，避免 B1:B1
    #非法引用）；矩阵 = 角标 + 各组按 span 分组合并。均为表头样式（粗体居中带框）
    header_cells = []
    if layout == "matrix" and groups:
        header_cells.append(xlsx_cell("A1", corner, False, STYLE_HEAD))
        col = 2   #B 列起
        for label, span in zip(groups, spans):
            span = max(1, int(span))
            start_l = col_letter(col - 1)
            end_l = col_letter(col - 1 + span - 1)
            header_cells.append(xlsx_cell("%s1" % start_l, label, False, STYLE_HEAD))
            #合并延续格也补带样式空格：合并区边框取自范围内各单元格，
            #只写锚点格会让合并带只剩首列一格宽的边框
            header_cells.extend(
                xlsx_cell("%s1" % col_letter(col - 1 + j), "", False, STYLE_HEAD)
                for j in range(1, span))
            if span > 1:
                merges.append('<mergeCell ref="%s1:%s1"/>' % (start_l, end_l))
            col += span
        col_count = col - 1
    else:
        header_cells.append(xlsx_cell("A1", corner, False, STYLE_HEAD))
        col_count = max([2] + [len(r) for r in rows])
        if title:
            header_cells.append(xlsx_cell("B1", title, False, STYLE_HEAD))
            #标题合并区延续格同上补齐（锚点 B1 之外 C1..末列）
            header_cells.extend(
                xlsx_cell("%s1" % col_letter(j), "", False, STYLE_HEAD)
                for j in range(2, col_count))
            if col_count >= 3:
                merges.append('<mergeCell ref="B1:%s1"/>' % col_letter(col_count - 1))
    xml_rows = ['<row r="1">%s</row>' % "".join(header_cells)]

    #数据行（带框样式；空单元格也写出——边框样式依赖单元格存在，缺格会露边框洞；
    #短行按表宽补齐空格，防全零统计等场景数据区列数残缺）
    r = 2
    for row in rows:
        cells = []
        for c in range(col_count):
            cell = str(row[c]) if c < len(row) else ""
            cells.append(xlsx_cell("%s%d" % (col_letter(c), r), cell,
                                   is_number(cell), STYLE_BODY))
        xml_rows.append('<row r="%d">%s</row>' % (r, "".join(cells)))
        r += 1

    #列宽（worksheet 子元素顺序：cols 在 sheetData 之前）
    cols_xml = _cols_xml()

    #合并单元格（worksheet 子元素顺序：sheetData 之后 mergeCells）
    merges_xml = ""
    if merges:
        merges_xml = '<mergeCells count="%d">%s</mergeCells>' % (len(merges), "".join(merges))

    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
            '%s<sheetData>%s</sheetData>%s</worksheet>'
            % (cols_xml, "".join(xml_rows), merges_xml))


def _cols_xml(rest_width=COL_WIDTH_REST):
    return ('<cols><col min="1" max="1" width="%d" customWidth="1"/>'
            '<col min="2" max="1024" width="%d" customWidth="1"/></cols>'
            % (COL_WIDTH_FIRST, rest_width))


#频段统计表冻结表头（222 行级长表滚动时列头常驻；真机 vtf.xlsx 复盘新增；
#组名行存在时冻结前 2 行，ySplit/topLeftCell 由调用方按表头行数格式化）
_BAND_FREEZE_XML = ('<sheetViews><sheetView workbookViewId="0">'
                    '<pane ySplit="%d" topLeftCell="A%d" activePane="bottomLeft" '
                    'state="frozen"/></sheetView></sheetViews>')


def build_band_sheet_xml(band, ctx):
    """频段统计表（第二 Sheet，add-curve-stats-band-count）：频段列头行 =
    角标+频段列头（表头样式，冻结表头），数据行 = 行标签（带框）+ 各频段计数
    （居中，计数>0 黄底标亮、其余居中带框）；矩阵布局且 Σspans=频段列数时
    首行与明细表同构（角标+组名合并块，2026-11-04 真机反馈），频段列头下移
    第 2 行、数据第 3 行起；行式/防御不匹配时退回单行头。列宽 45/12。
    band 契约见脚本头注释 band_stats。"""
    corner = "工况"
    columns = [str(c) for c in band.get("columns", [])]
    rows = band.get("rows", [])
    marks = band.get("marks", [])

    #组名合并行：仅矩阵布局且组块跨度合计与频段列数一致（明细 N 口径即
    #Σspans，正常导出恒成立）时启用，防御失配退回单行头
    groups = ctx.get("groups", []) if ctx.get("layout") == "matrix" else []
    spans = [max(1, int(s)) for s in ctx.get("spans", [])]
    use_groups = bool(groups) and sum(spans) == len(columns)
    header_row = 2 if use_groups else 1    #频段列头所在行
    data_start = header_row + 1
    merges = []

    xml_rows = []
    if use_groups:
        cells = [xlsx_cell("A1", corner, False, STYLE_HEAD)]
        col = 2   #B 列起，与明细表组块遍历同构
        for label, span in zip(groups, spans):
            start_l = col_letter(col - 1)
            end_l = col_letter(col - 1 + span - 1)
            cells.append(xlsx_cell("%s1" % start_l, str(label), False, STYLE_HEAD))
            #合并延续格补带样式空格（明细表同技法，防合并带边框残缺）
            cells.extend(
                xlsx_cell("%s1" % col_letter(col - 1 + j), "", False, STYLE_HEAD)
                for j in range(1, span))
            if span > 1:
                merges.append('<mergeCell ref="%s1:%s1"/>' % (start_l, end_l))
            col += span
        xml_rows.append('<row r="1">%s</row>' % "".join(cells))

    #频段列头行：有组名行时 A 列补空表头格保持表头块完整
    header_cells = []
    if use_groups:
        header_cells.append(xlsx_cell("A%d" % header_row, "", False, STYLE_HEAD))
    else:
        header_cells.append(xlsx_cell("A1", corner, False, STYLE_HEAD))
    for c, col_name in enumerate(columns):
        header_cells.append(
            xlsx_cell("%s%d" % (col_letter(c + 1), header_row), col_name, False, STYLE_HEAD))
    xml_rows.append('<row r="%d">%s</row>' % (header_row, "".join(header_cells)))

    r = data_start
    for row in rows:
        cells = [xlsx_cell("A%d" % r, str(row[0]) if row else "",
                           False, STYLE_BODY)]
        row_marks = marks[r - data_start] if r - data_start < len(marks) else []
        for c in range(len(columns)):
            value = row[c + 1] if c + 1 < len(row) else 0
            text = str(value)
            mark = bool(row_marks[c]) if c < len(row_marks) else False
            style = STYLE_BAND_MARK if mark else STYLE_BAND_BODY
            #计数契约（2026-11-04）：仅计数>0 是数值格写 <v>；空串（0 格与
            #缺席组槽位）→ 留空带框格
            cells.append(xlsx_cell("%s%d" % (col_letter(c + 1), r),
                                   text, bool(text), style))
        xml_rows.append('<row r="%d">%s</row>' % (r, "".join(cells)))
        r += 1

    merges_xml = ""
    if merges:
        merges_xml = '<mergeCells count="%d">%s</mergeCells>' % (len(merges), "".join(merges))

    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
            '%s%s<sheetData>%s</sheetData>%s</worksheet>'
            % (_BAND_FREEZE_XML % (data_start - 1, data_start),
               _cols_xml(BAND_COL_WIDTH), "".join(xml_rows), merges_xml))


def write_xlsx(ctx, output_path):
    sheet = build_sheet_xml(ctx)
    #频段统计表（第二 Sheet）：band_stats 缺席/无列头 = 只出明细单表（缺席即
    #跳过契约，宿主 generated=false 时不写该键）
    band = ctx.get("band_stats") or {}
    has_band = bool(band.get("columns"))
    band_sheet = build_band_sheet_xml(band, ctx) if has_band else ""
    content_types = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
        '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
        '<Default Extension="xml" ContentType="application/xml"/>'
        '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>'
        '<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
        + ('<Override PartName="/xl/worksheets/sheet2.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
           if has_band else "")
        + '<Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>'
        '</Types>')
    root_rels = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" '
        'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" '
        'Target="xl/workbook.xml"/>'
        '</Relationships>')
    workbook_sheets = ('<sheets><sheet name="曲线统计" sheetId="1" r:id="rId1"/>'
                       # rId3 才是 sheet2 的关系（rId2=styles，指错则第二表空白）
                       + ('<sheet name="频段统计" sheetId="2" r:id="rId3"/>'
                          if has_band else "")
                       + '</sheets>')
    workbook = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
        'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
        + workbook_sheets
        + '</workbook>')
    #既有关系不动（rId1 明细/rId2 styles），第二表追加 rId3——明细表零感知
    workbook_rels = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" '
        'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" '
        'Target="worksheets/sheet1.xml"/>'
        '<Relationship Id="rId2" '
        'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" '
        'Target="styles.xml"/>'
        + ('<Relationship Id="rId3" '
           'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" '
           'Target="worksheets/sheet2.xml"/>'
           if has_band else "")
        + '</Relationships>')

    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", content_types)
        z.writestr("_rels/.rels", root_rels)
        z.writestr("xl/workbook.xml", workbook)
        z.writestr("xl/_rels/workbook.xml.rels", workbook_rels)
        z.writestr("xl/styles.xml", STYLES_XML)
        z.writestr("xl/worksheets/sheet1.xml", sheet)
        if has_band:
            z.writestr("xl/worksheets/sheet2.xml", band_sheet)


# ---------------------------------------------------------------------------
# pptx 生成（单页表格幻灯片；包骨架/表格 XML 与 export_pptx.py 同源）
# ---------------------------------------------------------------------------

def shape_textbox(left, top, width, height, text, size_pt, bold=False, align=0, sid=2):
    paragraphs = []
    for line in (text or "").split("\n"):
        paragraphs.append(
            '<a:p><a:pPr algn="%s"/>'
            '<a:r><a:rPr lang="zh-CN" sz="%d" b="%d" dirty="0"/>'
            '<a:t>%s</a:t></a:r></a:p>'
            % ({1: "ctr", 2: "r"}.get(align, "l"), int(size_pt * 100), 1 if bold else 0, escape(line))
        )
    return (
        '<p:sp>'
        '<p:nvSpPr><p:cNvPr id="%d" name="TextBox%d"/><p:cNvSpPr txBox="1"/><p:nvPr/></p:nvSpPr>'
        '<p:spPr><a:xfrm><a:off x="%d" y="%d"/><a:ext cx="%d" cy="%d"/></a:xfrm>'
        '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom></p:spPr>'
        '<p:txBody><a:bodyPr wrap="square"><a:normAutofit/></a:bodyPr><a:lstStyle/>%s</p:txBody>'
        '</p:sp>'
    ) % (sid, sid, left, top, width, height, "".join(paragraphs))


def shape_table(left, top, width, height, header, rows, sid=2, header_spans=None,
                cell_fills=None, header2=None):
    #列数：分组合并表头时 = span 合计，否则取表头/数据行最大宽度
    if header_spans:
        col_count = sum(header_spans)
    else:
        col_count = max([len(header)] + ([len(header2)] if header2 else [])
                        + [len(r) for r in rows]) if (header or rows) else 1
    #header2 = 第二表头行（bold 无合并；第二页频段列头行，首行组名合并时用）
    row_count = len(rows) + (1 if header else 0) + (1 if header2 else 0)
    if row_count == 0:
        return ""

    col_w = width // col_count
    row_h = height // row_count
    cols_xml = "".join('<a:gridCol w="%d"/>' % col_w for _ in range(col_count))

    def make_cell(cell, bold, span=0, fill=None):
        txbody = (
            '<a:txBody><a:bodyPr/><a:lstStyle/>'
            '<a:p><a:pPr algn="ctr"/>'
            '<a:r><a:rPr lang="zh-CN" sz="1400" b="%d" dirty="0"/>'
            '<a:t>%s</a:t></a:r></a:p></a:txBody>'
        ) % (1 if bold else 0, escape(str(cell)))
        merge_open = ' gridSpan="%d"' % span if span > 1 else ""
        #填充仅标亮格写 solidFill（与既有单元格输出保持字节一致）；OOXML
        #tcPr 子元素顺序 fill 位于边框之后，无边框元素时直接 fill 合法
        if fill:
            tcpr = ('<a:tcPr anchor="ctr" marT="45720" marB="45720">'
                    '<a:solidFill><a:srgbClr val="%s"/></a:solidFill></a:tcPr>' % fill)
        else:
            tcpr = '<a:tcPr anchor="ctr" marT="45720" marB="45720"/>'
        return '<a:tc%s>%s%s</a:tc>' % (merge_open, txbody, tcpr)

    def make_row(cells, fills=None):
        filled = list(cells) + [""] * (col_count - len(cells))
        if fills is None:
            return '<a:tr h="%d">%s</a:tr>' % (row_h, "".join(
                make_cell(c, False) for c in filled))
        row_fills = list(fills) + [None] * (col_count - len(fills))
        return '<a:tr h="%d">%s</a:tr>' % (row_h, "".join(
            make_cell(c, False, 0, f) for c, f in zip(filled, row_fills)))

    def make_header_row(cells, spans):
        xml_cells = []
        for i, cell in enumerate(cells):
            span = spans[i] if spans and i < len(spans) else 1
            xml_cells.append(make_cell(cell, True, span))
            if span > 1:
                xml_cells.extend('<a:tc hMerge="1"/>' for _ in range(span - 1))
        return '<a:tr h="%d">%s</a:tr>' % (row_h, "".join(xml_cells))

    trs = []
    if header:
        trs.append(make_header_row(header, header_spans))
    if header2:
        trs.append(make_header_row(header2, None))
    for i, row in enumerate(rows):
        trs.append(make_row(row,
                            cell_fills[i] if cell_fills and i < len(cell_fills) else None))

    return (
        '<p:graphicFrame>'
        '<p:nvGraphicFramePr>'
        '<p:cNvPr id="%d" name="Table%d"/>'
        '<p:cNvGraphicFramePr><a:graphicFrameLocks noGrp="1"/></p:cNvGraphicFramePr>'
        '<p:nvPr/></p:nvGraphicFramePr>'
        '<p:xfrm><a:off x="%d" y="%d"/><a:ext cx="%d" cy="%d"/></p:xfrm>'
        '<a:graphic><a:graphicData uri="http://schemas.openxmlformats.org/drawingml/2006/table">'
        '<a:tbl><a:tblPr firstRow="%d" bandRow="1">'
        '<a:tableStyleId>{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}</a:tableStyleId>'
        '</a:tblPr><a:tblGrid>%s</a:tblGrid>%s</a:tbl>'
        '</a:graphicData></a:graphic></p:graphicFrame>'
    ) % (sid, sid, left, top, width, height, 1 if header else 0, cols_xml, "".join(trs))


def slide_xml(shapes_xml):
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" '
        'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" '
        'xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">'
        '<p:cSld><p:spTree>'
        '<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>'
        '<p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/>'
        '<a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>'
        '%s</p:spTree></p:cSld>'
        '<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>'
    ) % shapes_xml


MASTER_XML = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<p:sldMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" '
    'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" '
    'xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">'
    '<p:cSld><p:spTree>'
    '<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>'
    '<p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/>'
    '<a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>'
    '</p:spTree></p:cSld>'
    '<p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" '
    'accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" '
    'accent6="accent6" hlink="hlink" folHlink="folHlink"/>'
    '<p:sldLayoutIdLst><p:sldLayoutId id="2147483649" r:id="rId1"/></p:sldLayoutIdLst>'
    '</p:sldMaster>'
)

LAYOUT_XML = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<p:sldLayout xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" '
    'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" '
    'xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" '
    'type="blank" preserve="1" showMasterSp="0">'
    '<p:cSld name="Blank"><p:spTree>'
    '<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>'
    '<p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/>'
    '<a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>'
    '</p:spTree></p:cSld>'
    '<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>'
)

THEME_XML = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Office">'
    '<a:themeElements>'
    '<a:clrScheme name="Office">'
    '<a:dk1><a:sysClr val="windowText" lastClr="000000"/></a:dk1>'
    '<a:lt1><a:sysClr val="window" lastClr="FFFFFF"/></a:lt1>'
    '<a:dk2><a:srgbClr val="44546A"/></a:dk2>'
    '<a:lt2><a:srgbClr val="E7E6E6"/></a:lt2>'
    '<a:accent1><a:srgbClr val="4472C4"/></a:accent1>'
    '<a:accent2><a:srgbClr val="ED7D31"/></a:accent2>'
    '<a:accent3><a:srgbClr val="A5A5A5"/></a:accent3>'
    '<a:accent4><a:srgbClr val="FFC000"/></a:accent4>'
    '<a:accent5><a:srgbClr val="5B9BD5"/></a:accent5>'
    '<a:accent6><a:srgbClr val="70AD47"/></a:accent6>'
    '<a:hlink><a:srgbClr val="0563C1"/></a:hlink>'
    '<a:folHlink><a:srgbClr val="9563C1"/></a:folHlink>'
    '</a:clrScheme>'
    '<a:fontScheme name="Office">'
    '<a:majorFont><a:latin typeface="Calibri Light"/><a:ea typeface=""/><a:cs typeface=""/></a:majorFont>'
    '<a:minorFont><a:latin typeface="Calibri"/><a:ea typeface=""/><a:cs typeface=""/></a:minorFont>'
    '</a:fontScheme>'
    '<a:fmtScheme name="Office">'
    '<a:fillStyleLst>'
    '<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>'
    '<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>'
    '<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>'
    '</a:fillStyleLst>'
    '<a:lnStyleLst>'
    '<a:ln w="6350"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln>'
    '<a:ln w="12700"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln>'
    '<a:ln w="19050"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln>'
    '</a:lnStyleLst>'
    '<a:effectStyleLst>'
    '<a:effectStyle><a:effectLst/></a:effectStyle>'
    '<a:effectStyle><a:effectLst/></a:effectStyle>'
    '<a:effectStyle><a:effectLst/></a:effectStyle>'
    '</a:effectStyleLst>'
    '<a:bgFillStyleLst>'
    '<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>'
    '<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>'
    '<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>'
    '</a:bgFillStyleLst>'
    '</a:fmtScheme></a:themeElements></a:theme>'
)

TABLESTYLES_XML = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<a:tblStyles xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main">'
    '<a:tblStyle styleId="{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}" styleName="Medium Style 2 - Accent 1">'
    '<a:wholeTbl><a:tcPr>'
    '<a:lnL w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnL>'
    '<a:lnR w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnR>'
    '<a:lnT w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnT>'
    '<a:lnB w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnB>'
    '<a:solidFill><a:schemeClr val="accent1"/></a:solidFill></a:tcPr></a:wholeTbl>'
    '<a:band1H><a:tcPr><a:solidFill><a:schemeClr val="accent1"><a:tint val="20000"/></a:schemeClr></a:solidFill></a:tcPr></a:band1H>'
    '<a:band2H><a:tcPr><a:solidFill><a:schemeClr val="accent1"><a:tint val="40000"/></a:schemeClr></a:solidFill></a:tcPr></a:band2H>'
    '<a:band1V><a:tcPr><a:solidFill><a:schemeClr val="accent1"><a:tint val="20000"/></a:schemeClr></a:solidFill></a:tcPr></a:band1V>'
    '<a:band2V><a:tcPr><a:solidFill><a:schemeClr val="accent1"><a:tint val="40000"/></a:schemeClr></a:solidFill></a:tcPr></a:band2V>'
    '<a:firstCol><a:tcPr><a:lnL w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnL></a:tcPr></a:firstCol>'
    '<a:lastCol><a:tcPr><a:lnR w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnR></a:tcPr></a:lastCol>'
    '<a:firstRow><a:tcPr><a:lnB w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnB>'
    '<a:solidFill><a:schemeClr val="accent1"/></a:solidFill></a:tcPr></a:firstRow>'
    '<a:lastRow><a:tcPr><a:lnT w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnT>'
    '<a:solidFill><a:schemeClr val="accent1"/></a:solidFill></a:tcPr></a:lastRow>'
    '</a:tblStyle></a:tblStyles>'
)


def write_pptx(ctx, output_path):
    title = ctx.get("title", "")
    layout = ctx.get("layout", "rows")
    corner = ctx.get("corner", "工况")
    groups = ctx.get("groups", [])
    spans = ctx.get("spans", [])
    rows = ctx.get("rows", [])

    #表头：矩阵 = 角标 + 分组（gridSpan 合并）；行式 = 无表头行（设计稿 01-03，
    #标题信息由文本框承载）
    if layout == "matrix" and groups:
        header = [corner] + list(groups)
        header_spans = [1] + [max(1, int(s)) for s in spans]
    else:
        header = []
        header_spans = None

    shapes = []
    next_id = 2
    if title:
        shapes.append(shape_textbox(int(SLIDE_W * 0.05), int(SLIDE_H * 0.03),
                                    int(SLIDE_W * 0.9), int(SLIDE_H * 0.1),
                                    title, 28, bold=True, align=1, sid=next_id))
        next_id += 1
    #无标题（矩阵）时表格上移占满页幅
    table_top = int(SLIDE_H * 0.16) if title else int(SLIDE_H * 0.05)
    table_h = int(SLIDE_H * 0.78) if title else int(SLIDE_H * 0.92)
    table = shape_table(int(SLIDE_W * 0.05), table_top,
                        int(SLIDE_W * 0.9), table_h,
                        header, rows, sid=next_id, header_spans=header_spans)
    if table:
        shapes.append(table)

    #频段统计表（第二页，add-curve-stats-band-count）：标题"n-mHz频段统计"
    #+ 表格（首行角标+频段列头，firstRow 样式开），计数>0 格黄底标亮；
    #band_stats 缺席 = 单页（缺席即跳过契约）
    band = ctx.get("band_stats") or {}
    has_band = bool(band.get("columns"))
    slide2_shapes = []
    if has_band:
        band_corner = "工况"
        band_columns = [str(c) for c in band.get("columns", [])]
        band_rows = [[str(v) for v in r] for r in band.get("rows", [])]
        band_marks = band.get("marks", [])
        band_fills = []
        for i, row in enumerate(band_rows):
            marks_i = band_marks[i] if i < len(band_marks) else []
            band_fills.append([None] + [
                BAND_MARK_COLOR if (c < len(marks_i) and marks_i[c]) else None
                for c in range(len(band_columns))])
        band_title = "%g-%gHz频段统计" % (band.get("f_min", 0.0),
                                          band.get("f_max", 0.0))
        slide2_shapes.append(shape_textbox(int(SLIDE_W * 0.05), int(SLIDE_H * 0.03),
                                           int(SLIDE_W * 0.9), int(SLIDE_H * 0.1),
                                           band_title, 28, bold=True, align=1, sid=2))
        #矩阵布局组名合并首行（与 xlsx 第二表同构，2026-11-04 真机反馈）：
        #Σspans=频段列数时首行 = 角标+组名 gridSpan 合并、频段列头为第二表头行；
        #行式/防御失配退回单行频段头
        band_groups = ctx.get("groups", []) if ctx.get("layout") == "matrix" else []
        band_spans = [max(1, int(s)) for s in ctx.get("spans", [])]
        if band_groups and sum(band_spans) == len(band_columns):
            slide2_shapes.append(shape_table(
                int(SLIDE_W * 0.05), int(SLIDE_H * 0.16),
                int(SLIDE_W * 0.9), int(SLIDE_H * 0.78),
                [band_corner] + [str(g) for g in band_groups], band_rows,
                sid=3, cell_fills=band_fills,
                header_spans=[1] + band_spans,
                header2=[""] + band_columns))
        else:
            slide2_shapes.append(shape_table(int(SLIDE_W * 0.05), int(SLIDE_H * 0.16),
                                             int(SLIDE_W * 0.9), int(SLIDE_H * 0.78),
                                             [band_corner] + band_columns, band_rows,
                                             sid=3, cell_fills=band_fills))

    content_types = ['<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                     '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
                     '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
                     '<Default Extension="xml" ContentType="application/xml"/>'
                     '<Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>'
                     '<Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/>'
                     '<Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/>'
                     '<Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>'
                     '<Override PartName="/ppt/tableStyles.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.tableStyles+xml"/>'
                     '<Override PartName="/ppt/slides/slide1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>'
                     + ('<Override PartName="/ppt/slides/slide2.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>'
                        if has_band else "")
                     + '</Types>']
    presentation = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" '
        'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" '
        'xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" saveSubsetFonts="1">'
        '<p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rId1"/></p:sldMasterIdLst>'
        '<p:sldIdLst><p:sldId id="256" r:id="rId2"/>%s</p:sldIdLst>'
        '<p:sldSz cx="%d" cy="%d"/><p:notesSz cx="%d" cy="%d"/></p:presentation>'
    ) % ('<p:sldId id="257" r:id="rId4"/>' if has_band else "",
         SLIDE_W, SLIDE_H, SLIDE_H, SLIDE_W)
    pres_rels = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="slideMasters/slideMaster1.xml"/>'
        '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide" Target="slides/slide1.xml"/>'
        '<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="theme/theme1.xml"/>'
        + ('<Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide" Target="slides/slide2.xml"/>'
           if has_band else "")
        + '</Relationships>')
    root_rels = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>'
        '</Relationships>')
    master_rels = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>'
        '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="../theme/theme1.xml"/>'
        '</Relationships>')
    layout_rels = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="../slideMasters/slideMaster1.xml"/>'
        '</Relationships>')

    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", "".join(content_types))
        z.writestr("_rels/.rels", root_rels)
        z.writestr("ppt/presentation.xml", presentation)
        z.writestr("ppt/_rels/presentation.xml.rels", pres_rels)
        z.writestr("ppt/slideMasters/slideMaster1.xml", MASTER_XML)
        z.writestr("ppt/slideMasters/_rels/slideMaster1.xml.rels", master_rels)
        z.writestr("ppt/slideLayouts/slideLayout1.xml", LAYOUT_XML)
        z.writestr("ppt/slideLayouts/_rels/slideLayout1.xml.rels", layout_rels)
        z.writestr("ppt/theme/theme1.xml", THEME_XML)
        z.writestr("ppt/tableStyles.xml", TABLESTYLES_XML)
        z.writestr("ppt/slides/slide1.xml", slide_xml("".join(shapes)))
        z.writestr("ppt/slides/_rels/slide1.xml.rels",
                   '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                   '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                   '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>'
                   '</Relationships>')
        if has_band:
            z.writestr("ppt/slides/slide2.xml", slide_xml("".join(slide2_shapes)))
            z.writestr("ppt/slides/_rels/slide2.xml.rels",
                       '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                       '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                       '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>'
                       '</Relationships>')


def main():
    if len(sys.argv) < 3:
        sys.stderr.write("usage: export_stats.py <stats_ctx.json> <output.(xlsx|pptx)>\n")
        return 1
    json_path, output_path = sys.argv[1], sys.argv[2]
    try:
        with open(json_path, "r", encoding="utf-8") as f:
            ctx = json.load(f)
    except Exception as e:
        sys.stderr.write("stats export failed: %s\n" % e)
        return 1

    try:
        ext = os.path.splitext(output_path)[1].lower()
        if ext == ".xlsx":
            write_xlsx(ctx, output_path)
        elif ext == ".pptx":
            write_pptx(ctx, output_path)
        else:
            sys.stderr.write("stats export failed: unsupported output type %s\n" % ext)
            return 1
    except Exception as e:
        # 半成品包不留盘：失败时删除输出文件，避免宿主误判成功
        try:
            if os.path.exists(output_path):
                os.remove(output_path)
        except OSError:
            pass
        sys.stderr.write("stats export failed: %s\n" % e)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
