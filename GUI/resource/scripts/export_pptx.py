#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MxSim 报告导出 PowerPoint 生成脚本（纯标准库实现）
读取 JSON 格式的报告上下文，直接生成 OOXML 格式的 .pptx 包。
不依赖 python-pptx / lxml / Pillow，离线打包环境可用。

用法: python export_pptx.py <report_ctx.json> <output.pptx> [logo.png] [template.pptx]
      ctx 含 templateMode=true 时走模板化导出（template.pptx 为必传模板包，
      保留其页1封面、删除其余幻灯片；内容页 20261103-R13 起克隆模板自己的
      内容页样例（sldIdLst 序页2=勾选态/页3=不勾选态，保留装饰只换标题与
      图片区），无样例页回退内置几何）；
      否则与旧版行为一致（全量自建包）。
"""

import json
import re
import sys
import os
import zipfile
from xml.sax.saxutils import escape

# 13.333 x 7.5 英寸，EMU 单位（1 英寸 = 914400 EMU）
SLIDE_W = 12192000
SLIDE_H = 6858000

# 右上角 logo 尺寸与边距（EMU）
LOGO_W = 548640    # 0.6 英寸
LOGO_H = 274320    # 0.3 英寸
LOGO_MARGIN = 182880  # 0.2 英寸


# ---------------------------------------------------------------------------
# XML 片段生成
# ---------------------------------------------------------------------------

def align_str(align):
    """0=left, 1=center, 2=right"""
    return {1: "ctr", 2: "r"}.get(align, "l")


def shape_textbox(left, top, width, height, text, size_pt, bold=False, align=0, sid=2):
    """文本框形状；text 中的换行输出为多个段落"""
    paragraphs = []
    for i, line in enumerate(text.split("\n")):
        paragraphs.append(
            '<a:p><a:pPr algn="%s"/>'
            '<a:r><a:rPr lang="zh-CN" sz="%d" b="%d" dirty="0"/>'
            '<a:t>%s</a:t></a:r></a:p>'
            % (align_str(align), int(size_pt * 100), 1 if bold else 0, escape(line))
        )
    sp = (
        '<p:sp>'
        '<p:nvSpPr>'
        '<p:cNvPr id="%d" name="TextBox%d"/>'
        '<p:cNvSpPr txBox="1"/>'
        '<p:nvPr/>'
        '</p:nvSpPr>'
        '<p:spPr>'
        '<a:xfrm><a:off x="%d" y="%d"/><a:ext cx="%d" cy="%d"/></a:xfrm>'
        '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom>'
        '</p:spPr>'
        '<p:txBody>'
        '<a:bodyPr wrap="square"><a:normAutofit/></a:bodyPr>'
        '<a:lstStyle/>'
        '%s'
        '</p:txBody>'
        '</p:sp>'
    ) % (sid, sid, left, top, width, height, "".join(paragraphs))
    return sp


def shape_table(left, top, width, height, table_data, sid=2):
    """表格形状，支持 colSpan/rowSpan 合并"""
    rows_data = table_data.get("rows", [])
    row_count = len(rows_data)
    col_count = max(len(r) for r in rows_data) if rows_data else 1

    col_w = width // col_count
    cols_xml = "".join('<a:gridCol w="%d"/>' % col_w for _ in range(col_count))
    row_h = height // row_count if row_count else height

    # vmerge_pending[col] = 该列剩余的竖向合并行数
    vmerge_pending = {}
    rows_xml = []
    for row in rows_data:
        cells_xml = []
        c = 0
        while c < col_count:
            # 竖向合并的续格
            if vmerge_pending.get(c, 0) > 0:
                vmerge_pending[c] -= 1
                cells_xml.append('<a:tc vMerge="1"><a:txBody><a:bodyPr/><a:lstStyle/><a:p/></a:txBody>'
                                 '<a:tcPr anchor="ctr" marT="45720" marB="45720"/></a:tc>')
                c += 1
                continue
            if not row:
                # 数据不足时用空单元格补齐，保证每行的格子数与列数一致
                row = [{"text": ""}]
            cell = row.pop(0)
            row_span = int(cell.get("rowSpan", 1) or 1)
            col_span = int(cell.get("colSpan", 1) or 1)
            attrs = ""
            if col_span > 1:
                attrs += ' gridSpan="%d"' % col_span
            if row_span > 1:
                attrs += ' rowSpan="%d"' % row_span
                for k in range(col_span):
                    if c + k < col_count:
                        vmerge_pending[c + k] = row_span - 1
            bold = bool(cell.get("isHeader"))
            txbody = (
                '<a:txBody>'
                '<a:bodyPr/>'
                '<a:lstStyle/>'
                '<a:p><a:pPr algn="ctr"/>'
                '<a:r><a:rPr lang="zh-CN" sz="1400" b="%d" dirty="0"/>'
                '<a:t>%s</a:t></a:r></a:p>'
                '</a:txBody>'
            ) % (1 if bold else 0, escape(cell.get("text", "")))
            cells_xml.append('<a:tc%s>%s<a:tcPr anchor="ctr" marT="45720" marB="45720"/></a:tc>'
                             % (attrs, txbody))
            # 横向合并的续格
            for _ in range(1, col_span):
                if c + 1 < col_count:
                    cells_xml.append('<a:tc hMerge="1"><a:txBody><a:bodyPr/><a:lstStyle/><a:p/></a:txBody>'
                                     '<a:tcPr anchor="ctr" marT="45720" marB="45720"/></a:tc>')
                    if vmerge_pending.get(c + 1, 0) > 0:
                        pass  # 该列由本单元格的 rowSpan 覆盖
                    c += 1
            c += 1
        rows_xml.append('<a:tr h="%d">%s</a:tr>' % (row_h, "".join(cells_xml)))

    graphic_frame = (
        '<p:graphicFrame>'
        '<p:nvGraphicFramePr>'
        '<p:cNvPr id="%d" name="Table%d"/>'
        '<p:cNvGraphicFramePr><a:graphicFrameLocks noGrp="1"/></p:cNvGraphicFramePr>'
        '<p:nvPr/>'
        '</p:nvGraphicFramePr>'
        '<p:xfrm><a:off x="%d" y="%d"/><a:ext cx="%d" cy="%d"/></p:xfrm>'
        '<a:graphic><a:graphicData uri="http://schemas.openxmlformats.org/drawingml/2006/table">'
        '<a:tbl>'
        '<a:tblPr firstRow="1" bandRow="1">'
        '<a:tableStyleId>{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}</a:tableStyleId>'
        '</a:tblPr>'
        '<a:tblGrid>%s</a:tblGrid>'
        '%s'
        '</a:tbl>'
        '</a:graphicData>'
        '</a:graphic>'
        '</p:graphicFrame>'
    ) % (sid, sid, left, top, width, height, cols_xml, "".join(rows_xml))
    return graphic_frame


def shape_picture(left, top, width, height, rid, sid=2):
    """图片形状"""
    sp = (
        '<p:pic>'
        '<p:nvPicPr>'
        '<p:cNvPr id="%d" name="Picture%d"/>'
        '<p:cNvPicPr><a:picLocks noChangeAspect="1"/></p:cNvPicPr>'
        '<p:nvPr/>'
        '</p:nvPicPr>'
        '<p:blipFill><a:blip r:embed="%s"/><a:stretch><a:fillRect/></a:stretch></p:blipFill>'
        '<p:spPr>'
        '<a:xfrm><a:off x="%d" y="%d"/><a:ext cx="%d" cy="%d"/></a:xfrm>'
        '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom>'
        '</p:spPr>'
        '</p:pic>'
    ) % (sid, sid, rid, left, top, width, height)
    return sp


def shape_video(left, top, width, height, poster_rid, video_rid, media_rid, sid=2):
    """视频形状：海报帧 + videoFile 媒体引用，点击播放"""
    sp = (
        '<p:pic>'
        '<p:nvPicPr>'
        '<p:cNvPr id="%d" name="Video%d">'
        '<a:hlinkClick r:id="" action="ppaction://media"/>'
        '</p:cNvPr>'
        '<p:cNvPicPr><a:picLocks noChangeAspect="1"/></p:cNvPicPr>'
        '<p:nvPr>'
        '<a:videoFile r:link="%s"/>'
        '<p:extLst>'
        '<p:ext uri="{DAA4B4D4-6D71-4841-9C94-3DE7FCFB9230}">'
        '<p14:media xmlns:p14="http://schemas.microsoft.com/office/powerpoint/2010/main" r:embed="%s"/>'
        '</p:ext>'
        '</p:extLst>'
        '</p:nvPr>'
        '</p:nvPicPr>'
        '<p:blipFill><a:blip r:embed="%s"/><a:stretch><a:fillRect/></a:stretch></p:blipFill>'
        '<p:spPr>'
        '<a:xfrm><a:off x="%d" y="%d"/><a:ext cx="%d" cy="%d"/></a:xfrm>'
        '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom>'
        '</p:spPr>'
        '</p:pic>'
    ) % (sid, sid, video_rid, media_rid, poster_rid, left, top, width, height)
    return sp


def video_timing_xml(spid):
    """点击播放视频的 timing 树（PowerPoint 标准结构）"""
    return (
        '<p:timing>'
        '<p:tnLst>'
        '<p:par><p:cTn id="1" dur="indefinite" restart="never" nodeType="tmRoot"><p:childTnLst>'
        '<p:seq concurrent="1" nextAc="seek">'
        '<p:cTn id="2" dur="indefinite" nodeType="mainSeq"/>'
        '<p:prevCondLst><p:cond evt="onPrev" delay="0"><p:tgtEl><p:sldTgt/></p:tgtEl></p:cond></p:prevCondLst>'
        '<p:nextCondLst><p:cond evt="onNext" delay="0"><p:tgtEl><p:sldTgt/></p:tgtEl></p:cond></p:nextCondLst>'
        '<p:par><p:cTn id="3" fill="hold">'
        '<p:stCondLst><p:cond delay="indefinite"/><p:cond evt="onBegin" delay="0"><p:tn val="2"/></p:cond></p:stCondLst>'
        '<p:childTnLst>'
        '<p:par><p:cTn id="4" fill="hold">'
        '<p:stCondLst><p:cond delay="0"/></p:stCondLst>'
        '<p:childTnLst>'
        '<p:par><p:cTn id="5" presetID="1" presetClass="mediacall" presetSubtype="0" fill="hold" nodeType="clickEffect">'
        '<p:stCondLst><p:cond delay="0"/></p:stCondLst>'
        '<p:childTnLst>'
        '<p:cmd type="call" cmd="playFrom(0.0)">'
        '<p:cBhvr><p:cTn id="6" dur="indefinite"/><p:tgtEl><p:spTgt spid="%d"/></p:tgtEl></p:cBhvr>'
        '</p:cmd>'
        '</p:childTnLst>'
        '</p:cTn></p:par>'
        '</p:childTnLst>'
        '</p:cTn></p:par>'
        '</p:childTnLst>'
        '</p:cTn></p:par>'
        '</p:seq>'
        '</p:childTnLst></p:cTn></p:par>'
        '</p:tnLst>'
        '</p:timing>'
    ) % spid


def slide_xml(shapes_xml, timing_xml=""):
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" '
        'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" '
        'xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">'
        '<p:cSld><p:spTree>'
        '<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>'
        '<p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/>'
        '<a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>'
        '%s'
        '</p:spTree></p:cSld>'
        '<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>'
        '%s'
        '</p:sld>'
    ) % (shapes_xml, timing_xml)


# 关系类型
REL_TYPE = {
    "image": "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image",
    "video": "http://schemas.openxmlformats.org/officeDocument/2006/relationships/video",
    "media": "http://schemas.microsoft.com/office/2007/relationships/media",
}


def slide_rels_xml(image_rels):
    """image_rels: [(rid, media_file_name, kind)]，kind 旧格式为二元素时按 image 处理"""
    rels = ['<Relationship Id="rId1" '
            'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" '
            'Target="../slideLayouts/slideLayout1.xml"/>']
    for item in image_rels:
        rid, media = item[0], item[1]
        kind = item[2] if len(item) > 2 else "image"
        rels.append('<Relationship Id="%s" Type="%s" Target="../media/%s"/>'
                    % (rid, REL_TYPE[kind], media))
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '%s</Relationships>' % "".join(rels))


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
    '<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>'
    '</p:sldLayout>'
)

THEME_XML = (    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
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
    '<a:folHlink><a:srgbClr val="954F72"/></a:folHlink>'
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
    '</a:fmtScheme>'
    '</a:themeElements>'
    '</a:theme>'
)


TABLESTYLES_XML = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<a:tblStyles xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main">'
    '<a:tblStyle styleId="{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}" styleName="Medium Style 2 - Accent 1">'
    '<a:wholeTbl>'
    '<a:tcPr>'
    '<a:lnL w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnL>'
    '<a:lnR w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnR>'
    '<a:lnT w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnT>'
    '<a:lnB w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnB>'
    '<a:solidFill><a:schemeClr val="accent1"/></a:solidFill>'
    '</a:tcPr>'
    '</a:wholeTbl>'
    '<a:band1H><a:tcPr><a:solidFill><a:schemeClr val="accent1"><a:tint val="20000"/></a:schemeClr></a:solidFill></a:tcPr></a:band1H>'
    '<a:band2H><a:tcPr><a:solidFill><a:schemeClr val="accent1"><a:tint val="40000"/></a:schemeClr></a:solidFill></a:tcPr></a:band2H>'
    '<a:band1V><a:tcPr><a:solidFill><a:schemeClr val="accent1"><a:tint val="20000"/></a:schemeClr></a:solidFill></a:tcPr></a:band1V>'
    '<a:band2V><a:tcPr><a:solidFill><a:schemeClr val="accent1"><a:tint val="40000"/></a:schemeClr></a:solidFill></a:tcPr></a:band2V>'
    '<a:firstCol><a:tcPr><a:lnL w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnL></a:tcPr></a:firstCol>'
    '<a:lastCol><a:tcPr><a:lnR w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnR></a:tcPr></a:lastCol>'
    '<a:firstRow><a:tcPr><a:lnB w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnB><a:solidFill><a:schemeClr val="accent1"/></a:solidFill></a:tcPr></a:firstRow>'
    '<a:lastRow><a:tcPr><a:lnT w="12700" cap="flat" cmpd="sng" algn="ln"><a:solidFill><a:schemeClr val="lt1"/></a:solidFill></a:lnT></a:tcPr></a:lastRow>'
    '</a:tblStyle>'
    '</a:tblStyles>'
)


# ---------------------------------------------------------------------------
# 报告内容 -> 幻灯片
# ---------------------------------------------------------------------------

class PptxBuilder(object):
    def __init__(self, logo_path=""):
        self.slide_parts = []   # [(shapes_xml, image_rels, timing_xml)]
        self.media_used = {}    # src_path -> media file name
        self._pic_seq = 0
        self.logo_path = logo_path

    def add_slide(self, shapes_xml, image_rels=None, timing_xml=""):
        rels = list(image_rels or [])
        # 每页右上角贴 logo；id 必须页内唯一（内容形状占用 2~10），用 99 避免冲突
        if self.logo_path and os.path.exists(self.logo_path):
            media = self.media_name_for(self.logo_path)
            rid = "rId%d" % (len(rels) + 2)  # rId1 为 layout，媒体从 rId2 起
            shapes_xml += shape_picture(SLIDE_W - LOGO_MARGIN - LOGO_W, LOGO_MARGIN,
                                        LOGO_W, LOGO_H, rid, sid=99)
            rels.append((rid, media, "image"))
        self.slide_parts.append((shapes_xml, rels, timing_xml))

    def media_name_for(self, src_path):
        if src_path not in self.media_used:
            ext = os.path.splitext(src_path)[1].lower() or ".png"
            self.media_used[src_path] = "image%d%s" % (len(self.media_used) + 1, ext)
        return self.media_used[src_path]

    def next_pic_rid(self):
        # rId1 被 layout 占用，图片从 rId2 开始
        self._pic_seq += 1
        return "rId%d" % (self._pic_seq + 1)

    def save(self, output_path):
        content_types = ['<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                         '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
                         '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
                         '<Default Extension="xml" ContentType="application/xml"/>']
        exts = set()
        for _, rels, _timing in self.slide_parts:
            for _, media, _kind in rels:
                exts.add(os.path.splitext(media)[1].lstrip(".").lower())
        for ext in sorted(exts):
            mime = {"jpg": "image/jpeg", "jpeg": "image/jpeg",
                    "mp4": "video/mp4", "m4v": "video/mp4",
                    "avi": "video/x-msvideo",
                    "mpeg": "video/mpeg", "mpg": "video/mpeg"}.get(ext, "image/" + ext)
            content_types.append('<Default Extension="%s" ContentType="%s"/>' % (ext, mime))
        content_types.append('<Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>')
        content_types.append('<Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/>')
        content_types.append('<Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/>')
        content_types.append('<Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>')
        content_types.append('<Override PartName="/ppt/tableStyles.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.tableStyles+xml"/>')
        for i in range(len(self.slide_parts)):
            content_types.append('<Override PartName="/ppt/slides/slide%d.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>' % (i + 1))
        content_types.append('</Types>')

        # presentation.xml 与其 rels
        sld_ids = "".join('<p:sldId id="%d" r:id="rId%d"/>' % (256 + i, i + 2)
                          for i in range(len(self.slide_parts)))
        presentation = (
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" '
            'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" '
            'xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" '
            'saveSubsetFonts="1">'
            '<p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rId1"/></p:sldMasterIdLst>'
            '<p:sldIdLst>%s</p:sldIdLst>'
            '<p:sldSz cx="%d" cy="%d"/>'
            '<p:notesSz cx="%d" cy="%d"/>'
            '</p:presentation>'
        ) % (sld_ids, SLIDE_W, SLIDE_H, SLIDE_H, SLIDE_W)

        pres_rels = ['<Relationship Id="rId1" '
                     'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" '
                     'Target="slideMasters/slideMaster1.xml"/>']
        for i in range(len(self.slide_parts)):
            pres_rels.append('<Relationship Id="rId%d" '
                             'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide" '
                             'Target="slides/slide%d.xml"/>' % (i + 2, i + 1))
        pres_rels.append('<Relationship Id="rId%d" '
                         'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" '
                         'Target="theme/theme1.xml"/>' % (len(self.slide_parts) + 2))
        pres_rels.append('<Relationship Id="rId%d" '
                         'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/tableStyles" '
                         'Target="tableStyles.xml"/>' % (len(self.slide_parts) + 3))
        pres_rels_xml = ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                         '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                         '%s</Relationships>' % "".join(pres_rels))

        root_rels = ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                     '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                     '<Relationship Id="rId1" '
                     'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" '
                     'Target="ppt/presentation.xml"/>'
                     '</Relationships>')

        master_rels = ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                       '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                       '<Relationship Id="rId1" '
                       'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" '
                       'Target="../slideLayouts/slideLayout1.xml"/>'
                       '<Relationship Id="rId2" '
                       'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" '
                       'Target="../theme/theme1.xml"/>'
                       '</Relationships>')

        layout_rels = ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                       '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                       '<Relationship Id="rId1" '
                       'Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" '
                       'Target="../slideMasters/slideMaster1.xml"/>'
                       '</Relationships>')

        with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as z:
            z.writestr("[Content_Types].xml", "".join(content_types))
            z.writestr("_rels/.rels", root_rels)
            z.writestr("ppt/presentation.xml", presentation)
            z.writestr("ppt/_rels/presentation.xml.rels", pres_rels_xml)
            z.writestr("ppt/slideMasters/slideMaster1.xml", MASTER_XML)
            z.writestr("ppt/slideMasters/_rels/slideMaster1.xml.rels", master_rels)
            z.writestr("ppt/slideLayouts/slideLayout1.xml", LAYOUT_XML)
            z.writestr("ppt/slideLayouts/_rels/slideLayout1.xml.rels", layout_rels)
            z.writestr("ppt/theme/theme1.xml", THEME_XML)
            z.writestr("ppt/tableStyles.xml", TABLESTYLES_XML)
            for i, (shapes, rels, timing) in enumerate(self.slide_parts):
                z.writestr("ppt/slides/slide%d.xml" % (i + 1), slide_xml(shapes, timing))
                z.writestr("ppt/slides/_rels/slide%d.xml.rels" % (i + 1), slide_rels_xml(rels))
            # 媒体文件全局唯一，只写一次
            name_to_src = {v: k for k, v in self.media_used.items()}
            for media, src in name_to_src.items():
                z.write(src, "ppt/media/" + media)


def add_title_slide(b, meta):
    shapes = [shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.25), int(SLIDE_W * 0.8), int(SLIDE_H * 0.15),
                            meta.get("mainTitle", ""), 40, bold=True, align=1, sid=2)]
    if meta.get("showSubTitle") and meta.get("subTitle"):
        shapes.append(shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.45), int(SLIDE_W * 0.8), int(SLIDE_H * 0.1),
                                    meta["subTitle"], 24, align=1, sid=3))
    lines = []
    if meta.get("showAuthor"):
        lines.append("分析员: " + meta.get("author", ""))
    if meta.get("showCompany"):
        lines.append("公司: " + meta.get("company", ""))
    if meta.get("showAddress"):
        lines.append("地址: " + meta.get("address", ""))
    if meta.get("showPhone"):
        lines.append("电话: " + meta.get("phone", ""))
    lines.append("计算文件: " + meta.get("mxFileName", ""))
    lines.append("测试时间: " + meta.get("dateTime", ""))
    shapes.append(shape_textbox(int(SLIDE_W * 0.15), int(SLIDE_H * 0.6), int(SLIDE_W * 0.7), int(SLIDE_H * 0.3),
                                "\n".join(lines), 14, sid=4))
    b.add_slide("".join(shapes))


def add_section_slide(b, section):
    shapes = shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.4), int(SLIDE_W * 0.8), int(SLIDE_H * 0.2),
                           section.get("title", ""), 44, bold=True, align=1, sid=2)
    b.add_slide(shapes)


def add_section_title(shapes, section_title, sid=2):
    """内容页左上角的章节子标题（如"说明"、"模型"）"""
    if section_title:
        shapes.append(shape_textbox(int(SLIDE_W * 0.05), int(SLIDE_H * 0.03), int(SLIDE_W * 0.9), int(SLIDE_H * 0.08),
                                    section_title, 28, bold=True, align=0, sid=sid))


def add_text_slide(b, item, section_title=""):
    shapes = []
    add_section_title(shapes, section_title, sid=2)
    shapes.append(shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.15), int(SLIDE_W * 0.8), int(SLIDE_H * 0.7),
                                item.get("content", ""), 18, bold=bool(item.get("isBold")), sid=3))
    b.add_slide("".join(shapes))


def add_image_slide(b, item, section_title=""):
    path = item.get("imagePath", "")
    caption = item.get("caption", "")
    if not os.path.exists(path):
        add_text_slide(b, {"content": "图片丢失: " + path}, section_title)
        return
    media = b.media_name_for(path)
    rid = b.next_pic_rid()
    shapes = []
    add_section_title(shapes, section_title, sid=2)
    shapes.append(shape_picture(int(SLIDE_W * 0.1), int(SLIDE_H * 0.14), int(SLIDE_W * 0.8), int(SLIDE_H * 0.66),
                                rid, sid=3))
    if caption:
        shapes += shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.82), int(SLIDE_W * 0.8), int(SLIDE_H * 0.1),
                                caption, 18, align=1, sid=4)
    b.add_slide("".join(shapes), image_rels=[(rid, media, "image")])


def add_combined_images_slide(b, items, section_title=""):
    """一页网格摆放本节全部图片（"每个窗口导出至单独幻灯片"未勾选时）。
    单图独占整页版心；多图按两列排布，图题缩排在各图片下方"""
    imgs = [it for it in items
            if it.get("type") == 1 and os.path.exists(it.get("imagePath", ""))]
    if not imgs:
        return
    n = len(imgs)
    cols = 1 if n == 1 else 2
    rows = (n + cols - 1) // cols

    shapes = []
    add_section_title(shapes, section_title, sid=2)
    grid_top = int(SLIDE_H * 0.13)
    grid_h = int(SLIDE_H * 0.85) - grid_top
    grid_left = int(SLIDE_W * 0.05)
    grid_w = int(SLIDE_W * 0.9)
    cell_w = grid_w // cols
    cell_h = grid_h // rows
    cap_h = int(cell_h * 0.12)   # 图题条占单元格高度的一成

    rels = []
    for i, item in enumerate(imgs):
        r, c = divmod(i, cols)
        x = grid_left + c * cell_w
        y = grid_top + r * cell_h
        media = b.media_name_for(item.get("imagePath", ""))
        rid = b.next_pic_rid()
        rels.append((rid, media, "image"))
        sid = 10 + i   # 内容形状 id 页内唯一即可
        shapes.append(shape_picture(x + cell_w // 50, y,
                                    cell_w - cell_w // 25, cell_h - cap_h, rid, sid=sid))
        caption = item.get("caption", "")
        if caption:
            shapes += shape_textbox(x, y + cell_h - cap_h, cell_w, cap_h,
                                    caption, 12, align=1, sid=sid + 100)
    b.add_slide("".join(shapes), image_rels=rels)


def add_table_slide(b, item, section_title=""):
    table_data = item.get("table", {})
    title = table_data.get("title", "")
    rows_data = table_data.get("rows", [])

    shapes = []
    next_id = 2
    add_section_title(shapes, section_title, sid=next_id)
    if section_title:
        next_id += 1
    if title:
        shapes.append(shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.12), int(SLIDE_W * 0.8), int(SLIDE_H * 0.08),
                                    title, 24, bold=True, align=1, sid=next_id))
        next_id += 1
    if not rows_data:
        if len(shapes) > (1 if section_title else 0):
            b.add_slide("".join(shapes))
        return
    shapes.append(shape_table(int(SLIDE_W * 0.1), int(SLIDE_H * 0.21), int(SLIDE_W * 0.8), int(SLIDE_H * 0.67),
                              table_data, sid=next_id))
    b.add_slide("".join(shapes))


def extract_poster(video_path, base_name):
    """用 ffmpeg 从视频抽第一帧作海报帧；失败返回空串"""
    # 脚本位于 <程序目录>/scripts/，ffmpeg 在 <程序目录>/ffmpeg/ffmpeg.exe
    script_dir = os.path.dirname(os.path.abspath(__file__))
    app_dir = os.path.dirname(script_dir)
    ffmpeg = os.path.join(app_dir, "ffmpeg", "ffmpeg.exe")
    if not os.path.exists(ffmpeg):
        return ""
    poster = os.path.join(vdir_tmp(), base_name + "_poster.png")
    try:
        import subprocess
        subprocess.run([ffmpeg, "-y", "-loglevel", "error", "-i", video_path,
                        "-frames:v", "1", poster], timeout=30, check=True)
    except Exception:
        return ""
    return poster if os.path.exists(poster) else ""


def vdir_tmp():
    d = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_poster_tmp")
    os.makedirs(d, exist_ok=True)
    return d


def add_animation_slide(b, anim_path):
    """报告末尾的动画页：GIF 作为动图插入；AVI/MP4/MPEG 内嵌为可点击播放的视频"""
    if not anim_path or not os.path.exists(anim_path):
        return
    ext = os.path.splitext(anim_path)[1].lower().lstrip(".")

    if ext == "gif":
        # GIF 以图片形式插入，放映时自动播放
        media = b.media_name_for(anim_path)
        rid = b.next_pic_rid()
        shapes = [shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.03), int(SLIDE_W * 0.8), int(SLIDE_H * 0.08),
                                "结果动画", 28, bold=True, align=1, sid=2)]
        shapes.append(shape_picture(int(SLIDE_W * 0.15), int(SLIDE_H * 0.15), int(SLIDE_W * 0.7), int(SLIDE_H * 0.7),
                                    rid, sid=3))
        b.add_slide("".join(shapes), image_rels=[(rid, media, "image")])
        return

    # 视频格式：海报帧优先取引擎落盘的帧图片目录 <视频目录>/<文件名>/<文件名>_00000000.png
    vdir = os.path.dirname(anim_path) or "."
    base = os.path.splitext(os.path.basename(anim_path))[0]
    poster = os.path.join(vdir, base, base + "_00000000.png")
    if not os.path.exists(poster):
        candidates = sorted(f for f in os.listdir(vdir)
                            if f.startswith(base + "_") and f.lower().endswith((".png", ".jpeg")))
        if candidates:
            poster = os.path.join(vdir, candidates[0])
    # 帧目录不存在时用 ffmpeg 从视频抽第一帧作海报
    if not os.path.exists(poster):
        poster = extract_poster(anim_path, base)
    if not poster or not os.path.exists(poster):
        add_text_slide(b, {"content": "结果动画: " + anim_path})
        return

    poster_media = b.media_name_for(poster)
    video_media = b.media_name_for(anim_path)
    poster_rid, video_rid, media_rid = "rId2", "rId3", "rId4"

    shapes = [shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.03), int(SLIDE_W * 0.8), int(SLIDE_H * 0.08),
                            "结果动画", 28, bold=True, align=1, sid=2)]
    shapes.append(shape_video(int(SLIDE_W * 0.15), int(SLIDE_H * 0.15), int(SLIDE_W * 0.7), int(SLIDE_H * 0.7),
                              poster_rid, video_rid, media_rid, sid=3))
    shapes.append(shape_textbox(int(SLIDE_W * 0.1), int(SLIDE_H * 0.88), int(SLIDE_W * 0.8), int(SLIDE_H * 0.08),
                                "点击画面播放动画", 14, align=1, sid=4))
    rels = [(poster_rid, poster_media, "image"),
            (video_rid, video_media, "video"),
            (media_rid, video_media, "media")]
    b.add_slide("".join(shapes), image_rels=rels, timing_xml=video_timing_xml(3))


# ---------------------------------------------------------------------------
# 模板化导出（templateMode=true，20261029 报告输出模板化）
# 读报告母版包：保留页1（封面）原样，删除其余幻灯片，再按上下文逐视图页
# 追加内容页。内容页标题复刻母版标题占位符实测几何（EMU，解包 design/
# report-export/报告母版.pptx 量取）：
#   - 标题占位符（版式1 body idx=15）：(457200, 511475) 8229599x354012，20pt
#   - 内容插入框（页3 "整页一图"图框实测）：(641350, 1016000) 7200900x3632200
#     ——20261102-R08 起为勾选态宫格与不勾选态整图的统一划分基准
#     （567:286 = 7200900:3632200，12700 为最大公约数；母版页2 宫格实测几何
#     x=577850/3321050/6064250 行 y=1029335/2845435 单格 2336800x1816100
#     自 R08 起不再使用——宫格改为插入框整框按行列均分）
# 幻灯片尺寸沿用模板包 presentation.xml 的 sldSz（母版=9144000x5144135），
# 不用本脚本旧版的硬编码 12192000x6858000。母版自带版式装饰（logo 等），
# 模板分支不再另行贴 logo（旧分支行为不变）。
# 勾选态宫格 20261102-R06 起取消"超 6 图续页"：同节全部图窗排在同一张
# 幻灯片，行数=⌈N/C⌉。
# 20261102-R07 起列数跟随页面布局（节级 gridCols，0=自动 3 列）。
# 20261102-R08 拉伸充满（取代 R06/R07 等比居中，对应"逐字节一致"回归基线
# 作废、新基线=本次实现；3D 老链路字节回归红线不受影响）：勾选态格子=
# 插入框按 gridCols×行数均分、每图非等比拉伸充满其格子（单图独占整框）；
# 不勾选态整图 xfrm=插入框整框（宿主拼合画布已同框比例，无二次变形）。
# 20261103-R13 内容页克隆（取代"删除全部模板页后内置几何白纸重画"，后者
# 丢失用户自定义内容页上的装饰）：内容页改为克隆模板自己的内容页样例页
# （sldIdLst 序页2=勾选态样例、页3=不勾选态样例，只其一则两态共用，都缺
# → 内置几何原路径兜底）。克隆规则：标题占位符文本换页名（几何/字体原样）；
# 图片占位符 extent 作图片插入框（无则内置 TPL_PAGEIMG_XYWH 兜底）、本体
# 移除；普通 p:pic 与插入框相交删除（示例图腾出内容区）/不相交保留（装饰）；
# 其余形状（艺术字/文本框/组合等）原样保留；内容图片复用 R08 拉伸充满与
# 格子划分。默认母版同样走克隆（其页2/页3 即样例，产物几何与内置几何同值，
# 标题经版式 idx=15 继承）——R08 时代"内置几何"XML 字节基线随之作废
# （几何断言保留为回归线，新基线=克隆产物，workspace tools/r14_harness；
# 3D 老链路字节回归红线不受影响）。
# 20261104-R15 自定义模板内容页版式对齐母版观感（两处对齐修正）：①兜底
# 插入框 TPL_PAGEIMG_XYWH 与标准标题带 TPL_TITLE_XYWH 从母版画布量取的
# 绝对 EMU 改为按模板 sldSz 逐轴等比缩放（_tpl_scaled_xywh，四舍五入；
# 母版画布传入恒等=零回归；图片占位符派生框是用户自画的框,不缩放）——
# 任何画布≠母版的模板（4:3/16:9）内容图不再挤上部留大空白；②标题与插入
# 框相交时由 B05"重定位到框上方（保留原宽高字号——封面大字悬在页顶）"
# 改为规范化为缩放版标准标题带+页名 run 显式 sz=2000（_tpl_title_normalize,
# 置顶/timing 语义沿用 B05）,不相交零变化;20261104-B06 规范化补段落对齐：
# 页名 run 所在段 pPr 写显式 algn="l"（封面版式 lstStyle/母版 titleStyle
# 继承居中,几何对而文字带内居中;pPr 缺失构造插为 a:p 首子元素,已有子元素
# 只动属性）。
# ---------------------------------------------------------------------------

TPL_SLD_SZ = (9144000, 5144135)   # 母版画布（内置几何常量的量取基准,sldSz 解析失败回退值）
TPL_TITLE_XYWH = (457200, 511475, 8229599, 354012)
TPL_PAGEIMG_XYWH = (641350, 1016000, 7200900, 3632200)


def _tpl_scaled_xywh(xywh, cx, cy):
    """R15 画布缩放助手：内置几何常量按母版画布 TPL_SLD_SZ 实测量取,画布
    尺寸不同的自定义模板（如 4:3 9144000x6858000）逐轴等比缩放,四舍五入
    取整。母版画布传入时输出与输入逐位相等（恒等性,r14 断言自证）——
    默认母版导出零回归"""
    mx, my = TPL_SLD_SZ
    return (round(xywh[0] * cx / mx), round(xywh[1] * cy / my),
            round(xywh[2] * cx / mx), round(xywh[3] * cy / my))


def tpl_title_shape(text, sid=2, canvas=TPL_SLD_SZ):
    """内容页左上角标题：复用母版标题占位符（idx=15）继承版式字体样式，
    另写显式几何兜底（自定义模板缺该占位符时位置/尺寸仍正确）——R15 起
    显式几何按模板画布缩放（母版画布=原值逐位相等）"""
    tx, ty, tw, th = _tpl_scaled_xywh(TPL_TITLE_XYWH, canvas[0], canvas[1])
    return (
        '<p:sp><p:nvSpPr>'
        '<p:cNvPr id="%d" name="Title%d"/>'
        '<p:cNvSpPr><a:spLocks noGrp="1"/></p:cNvSpPr>'
        '<p:nvPr><p:ph type="body" sz="quarter" idx="15"/></p:nvPr>'
        '</p:nvSpPr>'
        '<p:spPr><a:xfrm><a:off x="%d" y="%d"/><a:ext cx="%d" cy="%d"/></a:xfrm>'
        '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom></p:spPr>'
        '<p:txBody><a:bodyPr/><a:lstStyle/>'
        '<a:p><a:r><a:rPr lang="zh-CN" altLang="en-US" sz="2000" dirty="0"/>'
        '<a:t>%s</a:t></a:r></a:p>'
        '</p:txBody></p:sp>'
    ) % (sid, sid, tx, ty, tw, th, escape(text))


class TemplateMediaNamer(object):
    """模板包媒体命名：避开母版已有 ppt/media/imageN，新图从最大编号后续号，
    全包唯一（同源图片只写一次）"""
    def __init__(self, pkg_names):
        max_no = 0
        for name in pkg_names:
            m = re.match(r"ppt/media/image(\d+)", name)
            if m:
                max_no = max(max_no, int(m.group(1)))
        self._next = max_no + 1
        self.used = {}   # src_path -> media file name

    def name_for(self, src_path):
        if src_path not in self.used:
            ext = os.path.splitext(src_path)[1].lower() or ".png"
            self.used[src_path] = "image%d%s" % (self._next, ext)
            self._next += 1
        return self.used[src_path]


def tpl_grid_slide(title, imgs, media, grid_cols=0, canvas=TPL_SLD_SZ):
    """宫格内容页（20261102-R08 拉伸充满）：格子=内容插入框 TPL_PAGEIMG_XYWH
    按列数×行数均分（列数=节级布局 gridCols，0=自动 3 列，R07 跟随页面布局
    语义不变），行数=⌈N/C⌉，同节全部图窗排在同一张幻灯片（20261102-R06 起
    取消超 6 图续页）。每图非等比拉伸充满其格子（取代 R06/R07 母版页2 实测
    宫格+fit_centered 等比居中——格子越接近图窗比例变形越小，用户已知并接受）；
    单图（N=1）独占整框（需求 R08 §1"单视图页该图直接拉伸充满插入框"）。
    图窗按序行主序填格，末行不满留白（空格留白）。格子边界按 EMU 整数累加
    分段（除不尽余量归末行/末列），格子并集恰为整框。imgs 须非空（调用方
    保证）。返回 (slide_xml, rels_xml)；rels 遵循脚本既有 rId 管理：
    rId1=layout，图片从 rId2 起。R15 起插入框/标题带按模板画布缩放
    （母版画布=原常量逐位相等）"""
    shapes = [tpl_title_shape(title, canvas=canvas)]
    rels = []
    cols = grid_cols if grid_cols > 0 else 3
    if len(imgs) == 1:
        cols = 1
    rows = (len(imgs) + cols - 1) // cols
    fx, fy, fw, fh = _tpl_scaled_xywh(TPL_PAGEIMG_XYWH, canvas[0], canvas[1])
    for i, item in enumerate(imgs):
        path = item.get("imagePath", "")
        col = i % cols
        row = i // cols
        x0 = fx + col * fw // cols
        x1 = fx + (col + 1) * fw // cols
        y0 = fy + row * fh // rows
        y1 = fy + (row + 1) * fh // rows
        media_name = media.name_for(path)
        rid = "rId%d" % (len(rels) + 2)
        rels.append((rid, media_name, "image"))
        shapes.append(shape_picture(x0, y0, x1 - x0, y1 - y0, rid, sid=10 + i))
    return slide_xml("".join(shapes)), slide_rels_xml(rels)


def tpl_pageimage_slide(title, item, media, canvas=TPL_SLD_SZ):
    """整页大图内容页（对齐母版页3图框）：20261102-R08 起整图非等比拉伸
    充满插入框整框（宿主拼合画布已同框比例，无二次变形；取代 fit_centered
    等比缩放置中）。R15 起插入框/标题带按模板画布缩放"""
    path = item.get("imagePath", "")
    shapes = [tpl_title_shape(title, canvas=canvas)]
    rels = []
    x, y, w, h = _tpl_scaled_xywh(TPL_PAGEIMG_XYWH, canvas[0], canvas[1])
    media_name = media.name_for(path)
    rid = "rId%d" % (len(rels) + 2)
    rels.append((rid, media_name, "image"))
    shapes.append(shape_picture(x, y, w, h, rid, sid=3))
    return slide_xml("".join(shapes)), slide_rels_xml(rels)


def read_package(path):
    """读 zip 包全部非目录条目为 {name: bytes}"""
    pkg = {}
    with zipfile.ZipFile(path, "r") as z:
        for info in z.infolist():
            if info.filename.endswith("/"):
                continue
            pkg[info.filename] = z.read(info.filename)
    return pkg


# ---------------------------------------------------------------------------
# 内容页克隆（20261103-R13）：样例页提取/逐形状处理/装饰保留
# ---------------------------------------------------------------------------

_XML_TAG_RE = re.compile(
    r"<(/?)([A-Za-z_][\w.:-]*)"            # 1=闭标记 2=标签名
    r"((?:\"[^\"]*\"|'[^']*'|[^\"'>])*?)"  # 3=属性区（惰性,引号内的 '>' 不越界,
                                           #   末尾 '/' 不被吞入,留给自闭合组）
    r"(/?)>")                               # 4=自闭合标记


def _toplevel_spans(xml):
    """XML 片段顶层元素 span 列表 [(标签名, start, end)]：全标签扫描+栈式
    深度配对（同名/异名嵌套、自闭合、属性值内 '>' 均正确处理；注释/CDATA
    不支持——机器生成的 slide XML 不含，异常时抛 ValueError 走克隆失败
    链路回退默认母版）"""
    stack = []      # [(标签名, 开标签起点)]
    spans = []
    for m in _XML_TAG_RE.finditer(xml):
        closing, name, _attrs, selfclose = m.groups()
        if selfclose:
            if not stack:
                spans.append((name, m.start(), m.end()))
            continue
        if closing:
            if not stack or stack[-1][0] != name:
                raise ValueError("unbalanced end tag </%s>" % name)
            start = stack.pop()[1]
            if not stack:
                spans.append((name, start, m.end()))
        else:
            stack.append((name, m.start()))
    if stack:
        raise ValueError("unclosed tag <%s>" % stack[-1][0])
    return spans


def _tpl_ph_attrs(block):
    """形状块内占位符标记 <p:ph .../> 的属性串；非占位符形状返回 None
    （裸 <p:ph/> 组恒参与匹配返回空串=obj 占位符，按内容占位符处理）"""
    m = re.search(r"<p:ph((?:\s[^>]*?)*)/?>", block)
    return m.group(1) if m else None


def _tpl_block_text(block):
    """块内是否含非空文本（内容占位符为空=PowerPoint"单击图标添加"态）"""
    for t in re.findall(r"<a:t>(.*?)</a:t>", block, re.S):
        if t.strip():
            return True
    return False


def _tpl_ph_kind(attrs, block):
    """占位符类别：'title'=标题占位符（type=title/ctrTitle，或母版约定
    body idx=15——与 R07/R08 内置路径同源）；'pic'=图片占位符（type=pic；
    或无文本的内容占位符 body/obj idx≠15，其 extent 即用户意图的插图区）；
    其余（页脚/日期/有文字的内容占位符等）'other' 按装饰保留"""
    m = re.search(r'type="([A-Za-z]+)"', attrs)
    t = m.group(1) if m else "obj"
    im = re.search(r'idx="(\d+)"', attrs)
    idx = im.group(1) if im else ""
    if t in ("title", "ctrTitle"):
        return "title"
    if t == "body" and idx == "15":
        return "title"
    if t == "pic":
        return "pic"
    if t in ("body", "obj") and not _tpl_block_text(block):
        return "pic"
    return "other"


def _tpl_shape_frame(block):
    """形状显式几何 <a:xfrm><a:off/><a:ext/></a:xfrm> → (x, y, w, h)；
    无显式几何（继承版式）返回 None"""
    m = re.search(r'<a:xfrm[^>]*>\s*<a:off x="(-?\d+)" y="(-?\d+)"\s*/>'
                  r'\s*<a:ext cx="(-?\d+)" cy="(-?\d+)"\s*/>', block)
    if not m:
        return None
    return tuple(int(v) for v in m.groups())


def _rects_intersect(a, b):
    """矩形相交（边贴合不算——相邻格子不误删）"""
    ax, ay, aw, ah = a
    bx, by, bw, bh = b
    return ax < bx + bw and bx < ax + aw and ay < by + bh and by < ay + ah


def _tpl_set_title_text(block, text):
    """标题占位符文本替换为页名：首个 run 只换 <a:t> 文本（rPr 字体/字号
    原样），其余 run 移除（清掉样例残留标题，如母版"Non Struct Disp-1"）。
    空占位符（无 run——封面式 ctrTitle/subTitle 段落只有 pPr+endParaRPr，
    B04 实证）向首段构造 run 插入：位置在段尾 endParaRPr 之前（OOXML
    a:p 子元素顺序 pPr→run*→endParaRPr，插在其后会触发 PowerPoint 修复
    丢弃页名），rPr 继承 endParaRPr 的属性与字体子元素（保留原格式，
    如 sz=4400/Arial/宋体；lang 归一 zh-CN）；无段落时补完整段落。
    sp 几何/段落结构原样。无 txBody 原块返回"""
    tb_m = re.search(r"<p:txBody>(.*?)</p:txBody>", block, re.S)
    if not tb_m:
        return block
    tb = tb_m.group(1)
    esc = escape(text)
    runs = list(re.finditer(r"<a:r>.*?</a:r>", tb, re.S))
    if runs:
        first = runs[0]
        r = first.group(0)
        if re.search(r"<a:t>.*?</a:t>", r, re.S):
            r = re.sub(r"<a:t>.*?</a:t>", lambda _: "<a:t>%s</a:t>" % esc,
                       r, count=1, flags=re.S)
        else:
            r = r[:-len("</a:r>")] + "<a:t>%s</a:t></a:r>" % esc
        pieces = [tb[:first.start()], r]
        pos = first.end()
        for m in runs[1:]:
            pieces.append(tb[pos:m.start()])
            pos = m.end()
        pieces.append(tb[pos:])
        tb_new = "".join(pieces)
    else:
        # 无 run（空占位符）：首段构造 run，插在段尾 endParaRPr 之前
        p_m = re.search(r"<a:p>.*?</a:p>", tb, re.S)
        if not p_m:
            tb_new = tb + ('<a:p><a:r><a:rPr lang="zh-CN" altLang="en-US"/>'
                           "<a:t>%s</a:t></a:r></a:p>" % esc)
        else:
            seg = p_m.group(0)
            epr = re.search(r"<a:endParaRPr\b[^>]*(?:/>|>.*?</a:endParaRPr>)",
                            seg, re.S)
            if epr:
                whole = epr.group(0)
                am = re.match(r"<a:endParaRPr\b([^>]*?)/?>", whole)
                # lang/altLang 一并剥离（endParaRPr 常见 lang="en-US"
                # altLang="zh-CN"，构造的 rPr 头部自带二者，漏剥任一即
                # 属性重复→XML 非 well-formed，评审阻塞项）
                attrs = re.sub(r'\s+(?:lang|altLang)="[^"]*"', "", am.group(1))
                kids = ("" if whole.endswith("/>")
                        else whole[am.end():whole.rfind("</a:endParaRPr>")])
                rpr = ('<a:rPr lang="zh-CN" altLang="en-US"%s>%s</a:rPr>'
                       % (attrs, kids) if kids else
                       '<a:rPr lang="zh-CN" altLang="en-US"%s/>' % attrs)
                run = "<a:r>%s<a:t>%s</a:t></a:r>" % (rpr, esc)
                seg = seg[:epr.start()] + run + seg[epr.start():]
            else:
                seg = (seg[:-len("</a:p>")]
                       + '<a:r><a:rPr lang="zh-CN" altLang="en-US"/>'
                         "<a:t>%s</a:t></a:r></a:p>" % esc)
            tb_new = tb[:p_m.start()] + seg + tb[p_m.end():]
    return block[:tb_m.start(1)] + tb_new + block[tb_m.end(1):]


def _tpl_title_normalize(block, canvas):
    """R15：标题生效几何与插入框相交时，规范化为标准标题带（取代 B05
    "重定位到框上方"——封面式标题原宽高/字号保留会以封面大字号悬在页顶，
    用户图3 明示期望左上标准标题）——写显式 xfrm=缩放版 TPL_TITLE_XYWH
    （同内置 tpl_title_shape 标题带，母版画布=原值），页名 run 显式
    sz="2000"（20pt 对齐内置标题；封面大字来自占位符样式/继承的
    endParaRPr，显式 sz 覆盖，endParaRPr 不动）。置顶/timing 语义沿用
    B05：同一形状移位、id 不变、不置 removed、样例 p:timing 保留。
    替换路径保留原 xfrm 开标签属性（rot/flipH/flipV 等），匹配止于
    a:ext 自闭合、原闭合 </a:xfrm> 保留；注入路径（无显式几何，展开/
    插入 spPr）无原闭合可依赖，必须使用完整闭合形态（B05 评审返工教训：
    缺 </a:xfrm> 曾致非法 XML）。返回规范化后的标题 sp 块。
    B06 补段落对齐规范化：封面版式 lstStyle/母版 titleStyle 常为
    algn="ctr"，页名段 pPr 无显式 algn 时继承居中（实机目检：几何已入
    左上标准带但文字带内居中）——锚定页名 run 所在段（不盲改首段/其余
    段，模板作者自设段落样式原样保留）写显式 algn="l"：pPr 已有 algn
    属性改写为 l、无 algn 属性追加；pPr 不存在构造 <a:pPr algn="l"/>
    插为该 a:p 首子元素（schema 顺序 pPr 必居首）；pPr 已有子元素
    （buClrTx/buNone 等）只动开标签属性、子元素原样。无页名 run（无
    txBody 等异常态）不动作。仅相交规范化路径生效——不相交标题零变化"""
    off_ext = '<a:off x="%d" y="%d"/><a:ext cx="%d" cy="%d"/>' % (
        _tpl_scaled_xywh(TPL_TITLE_XYWH, canvas[0], canvas[1]))
    m = re.search(r"(<a:xfrm[^>]*>)\s*<a:off[^>]*/>\s*<a:ext[^>]*/>", block)
    if m:
        block = block[:m.start()] + m.group(1) + off_ext + block[m.end():]
    else:
        xfrm = "<a:xfrm>%s</a:xfrm>" % off_ext
        if "<p:spPr/>" in block:      # 空自闭合 spPr（继承版式标题）展开注入
            block = block.replace("<p:spPr/>", "<p:spPr>%s</p:spPr>" % xfrm, 1)
        else:
            m = re.search(r"<p:spPr>", block)
            if m:
                block = block[:m.end()] + xfrm + block[m.end():]
            # 无 spPr 的异常占位符：几何无法落位,仅走下方字号规范化
    rm = re.search(r"<a:r>.*?</a:r>", block, re.S)
    if rm:
        run = rm.group(0)
        pm = re.search(r"<a:rPr\b([^>]*?)(/?>)", run)
        if pm:                        # 已有 rPr：剥旧 sz 写标准 20pt（字体子元素原样）
            attrs = re.sub(r'\s+sz="\d+"', "", pm.group(1))
            run = (run[:pm.start()] + '<a:rPr%s sz="2000"%s' % (attrs, pm.group(2))
                   + run[pm.end():])
        else:                         # 无 rPr 的裸 run：补最小 rPr
            run = run.replace(
                "<a:r>",
                '<a:r><a:rPr lang="zh-CN" altLang="en-US" sz="2000" dirty="0"/>', 1)
        block = block[:rm.start()] + run + block[rm.end():]
    # B06：段落对齐规范化——锚定页名 run 所在段（a:p 不嵌套,非贪婪配对即
    # 段边界）写显式 algn="l"。pPr 已有子元素（buClrTx/buNone 等）只动开
    # 标签属性、子元素原样；algn 属性剥旧写新一并完成（防属性重复）。
    rm = re.search(r"<a:r>.*?</a:r>", block, re.S)
    if rm:
        for pg in re.finditer(r"<a:p>.*?</a:p>", block, re.S):
            if not (pg.start() <= rm.start() < pg.end()):
                continue              # 页名 run 不在本段,跳过
            seg = pg.group(0)
            gm = re.search(r"<a:pPr\b([^>]*?)(/?>)", seg)
            if gm:                    # pPr 在：改写/追加 algn 属性
                attrs = re.sub(r'\s+algn="[^"]*"', "", gm.group(1)) + ' algn="l"'
                seg = (seg[:gm.start()] + "<a:pPr%s%s" % (attrs, gm.group(2))
                       + seg[gm.end():])
            else:                     # pPr 缺失：构造插为 a:p 首子元素
                om = re.search(r"<a:p>", seg)
                seg = seg[:om.end()] + '<a:pPr algn="l"/>' + seg[om.end():]
            block = block[:pg.start()] + seg + block[pg.end():]
            break
    return block


def tpl_clone_slide(sample_xml, sample_rels, title, imgs, media,
                    grid_cols=0, image_per_chart=True, canvas=TPL_SLD_SZ):
    """克隆样例页生成内容页（20261103-R13）。两遍扫描：先定位图片占位符
    extent 定框（与形状出现顺序/z 序无关），再统一按定框逐顶层形状分类
    处理样例页 spTree：
      - 标题占位符：文本替换为页名（首个标题占位符生效，空占位符也构造
        run 写入——B04 封面式 ctrTitle，插在段尾 endParaRPr 之前并继承其
        格式）；标题生效几何与插入框相交时（B05 封面式居中标题落在内容
        区内，无显式几何按 TPL_TITLE_XYWH 原值判定）规范化为标准标题带
        （R15：显式 xfrm=缩放版 TPL_TITLE_XYWH+页名 run 显式 sz="2000"，
        B06：页名段 pPr 显式 algn="l"（封面版式继承居中纠正），
        取代 B05"重定位到框上方"——封面大字原样保留会悬在页顶）并置顶
        （spTree 末尾=最上层，不被生成图覆盖），不相交原位保留（默认
        母版零变化）；
      - 图片占位符：首个显式 xfrm extent 作为图片插入框（R15 用户自画的
        框原样尊重,不缩放）、本体移除；无则内置 TPL_PAGEIMG_XYWH 兜底
        （R15 起兜底框按模板画布缩放——母版画布量取的绝对 EMU 在其它
        画布上会挤上部留大空白）；
      - 普通 p:pic（非占位符示例图）：与插入框相交删除（腾出内容区）/
        不相交保留（装饰）；几何缺失（继承版式）按相交处理——宁删勿与
        内容区叠压；
      - 其余占位符（页脚/日期/封面式空 subTitle 等）无实际文本 → 删除
        （B04 消除空提示虚线框），有文本按装饰保留；
      - 其余形状（文本框/艺术字/组合/表格等）原样保留；
      - 内容图片沿用 R08 拉伸充满：插入框按 gridCols×⌈N/C⌉ 整数累加均分
        （单图/不勾选态独占整框），形状 id 接本页现存最大 cNvPr id 续号、
        rId 接样例页 rels 最大 rId 续号（每页 rels 独立，防撞名）；
      - 样例页 rels 原样拷贝为新页 rels 基底（版式/tags/装饰图引用继续
        有效，目标部件 media/tags/layouts 留包不删），新图片关系追加其后；
        样例页 rels 缺失时按既有生成器合成（rId1=layout）；
      - 本页删过形状且样例带 p:timing 时丢弃 timing（悬空 spid 会触发
        PowerPoint 修复）。
    canvas=(sldSz cx, cy)（R15，缺省=母版画布）。
    处理异常向上抛出（渲染器既有 stderr → 回退默认母版链路兜底）。
    返回 (slide_xml, rels_xml)。"""
    sp_m = re.search(r"<p:spTree>(.*?)</p:spTree>", sample_xml, re.S)
    if not sp_m:
        raise ValueError("template sample slide has no spTree")
    inner = sp_m.group(1)
    spans = _toplevel_spans(inner)

    # 第一遍：定框——图片占位符首个显式 xfrm extent 生效（定框与形状出现
    # 顺序无关，z 序不影响；R15 用户自画的框原样尊重,不缩放）；无显式几何
    # 的图片占位符不作插入框；无任何可用占位符几何 → 内置 TPL_PAGEIMG_XYWH
    # 兜底（R15 起按模板画布缩放）
    insert_frame = None
    for name, start, end in spans:
        if name not in ("p:sp", "p:pic"):
            continue
        block = inner[start:end]
        attrs = _tpl_ph_attrs(block)
        if attrs is None or _tpl_ph_kind(attrs, block) != "pic":
            continue
        frame = _tpl_shape_frame(block)
        if frame is not None:
            insert_frame = frame
            break
    if insert_frame is None:
        insert_frame = _tpl_scaled_xywh(TPL_PAGEIMG_XYWH, canvas[0], canvas[1])

    # 第二遍：统一按定框分类定处理动作（有序替换表）——相交判定与形状
    # 出现顺序解耦（普通 p:pic 先于图片占位符出现时同样按占位符框判定，
    # 而非内置框）
    actions = []           # [(start, end, replacement)]
    removed = False
    title_done = False
    title_block = None     # B05:相交时重定位后的标题 sp(置顶追加 spTree 末尾)
    for name, start, end in spans:
        if name not in ("p:sp", "p:pic"):
            continue       # 组合/表格等其余顶层形状原样保留
        block = inner[start:end]
        attrs = _tpl_ph_attrs(block)
        if attrs is not None:                    # 占位符实例
            kind = _tpl_ph_kind(attrs, block)
            if kind == "title" and not title_done:
                title_done = True
                new_block = _tpl_set_title_text(block, title)
                # B05:标题生效几何与插入框相交(封面式居中标题落在内容区内,
                # 无显式几何按 TPL_TITLE_XYWH 原值判定)→ 原位移除,规范化为
                # 标准标题带(R15:缩放版 TPL_TITLE_XYWH+页名 run sz=2000,
                # 取代 B05 重定位框上方——封面大字原样会悬在页顶)并置顶追加
                # (spTree 末尾=最上层,绘制在生成图之上);不相交原位替换
                # (默认母版标题在框上方,零变化回归线)
                tf = _tpl_shape_frame(block) or TPL_TITLE_XYWH
                if _rects_intersect(tf, insert_frame):
                    actions.append((start, end, ""))
                    # 不置 removed：规范化是同一形状改写（id 不变）,样例
                    # timing 的 spid 引用仍有效,仅标题规范化不丢 p:timing
                    title_block = _tpl_title_normalize(new_block, canvas)
                else:
                    actions.append((start, end, new_block))
            elif kind == "pic" and _tpl_shape_frame(block) is not None:
                actions.append((start, end, ""))   # 图片占位符本体移除
                removed = True
            elif not _tpl_block_text(block):
                # 其余占位符（页脚/日期/封面式空 subTitle/无显式几何的图片
                # 占位符等，B04）无实际文本 → 删除（消除 PowerPoint 空提示
                # 虚线框）；有文本按装饰保留
                actions.append((start, end, ""))
                removed = True
            continue
        if name == "p:pic":  # 普通图片形状（示例图/装饰图）
            frame = _tpl_shape_frame(block)
            if frame is None or _rects_intersect(frame, insert_frame):
                actions.append((start, end, ""))       # 相交=示例图，删除
                removed = True
            # 不相交=装饰图，原样保留

    # 内容图片：R08 拉伸充满格子划分（不勾选态/单图=插入框整框；
    # 插入框定框阶段已保证非 None——占位符 extent 或缩放兜底框）
    fx, fy, fw, fh = insert_frame
    n = len(imgs)
    cols = 1
    if image_per_chart and n > 1:
        cols = grid_cols if grid_cols > 0 else 3
    rows = (n + cols - 1) // cols
    rels = []
    rid_seq = max([int(x) for x in re.findall(r'Id="rId(\d+)"', sample_rels)] or [1])

    # 先重建 spTree（装饰保留/占位符处理完）再取形状 id 续号基线；
    # 置顶追加的标题原位移除后不在 core_inner,其 id（如用户模板 3074
    # 为全页最大）须一并计入,防与生成图撞 id
    pieces = []
    pos = 0
    for start, end, repl in actions:
        pieces.append(inner[pos:start])
        pieces.append(repl)
        pos = end
    pieces.append(inner[pos:])
    core_inner = "".join(pieces)
    sid_src = core_inner + (title_block or "")
    sid_seq = max([int(x) for x in re.findall(r'<p:cNvPr id="(\d+)"', sid_src)] or [1])

    gen = []
    for i, item in enumerate(imgs):
        path = item.get("imagePath", "")
        col, row = i % cols, i // cols
        x0 = fx + col * fw // cols
        x1 = fx + (col + 1) * fw // cols
        y0 = fy + row * fh // rows
        y1 = fy + (row + 1) * fh // rows
        rid = "rId%d" % (rid_seq + 1 + len(rels))
        rels.append((rid, media.name_for(path), "image"))
        gen.append(shape_picture(x0, y0, x1 - x0, y1 - y0, rid, sid=sid_seq + 1 + i))

    new_xml = (sample_xml[:sp_m.start(1)] + core_inner + "".join(gen)
               + (title_block or "") + sample_xml[sp_m.end(1):])
    if removed:
        new_xml = re.sub(r"<p:timing>.*?</p:timing>", "", new_xml, flags=re.S)

    # rels：样例页 rels 原样拷贝为基底（版式/tags/装饰图引用继续有效），
    # 新图片关系追加其后；样例无 rels 时按既有生成器合成（rId1=layout）
    if "</Relationships>" in sample_rels:
        add = "".join('<Relationship Id="%s" Type="%s" Target="../media/%s"/>'
                      % (rid, REL_TYPE["image"], mn) for rid, mn, _k in rels)
        rels_xml = sample_rels.replace("</Relationships>", add + "</Relationships>")
    else:
        rels_xml = slide_rels_xml(rels)
    return new_xml, rels_xml


def _tpl_sample_page(pkg, pres_rels, entry):
    """sldIdLst 条目 → 样例页 (slide_xml, rels_xml) 文本对。条目缺失/关系
    或部件缺失（包结构异常的定位级）返回 None——该态回退内置几何原路径；
    部件存在但克隆处理失败则向上抛（渲染器 stderr → 回退默认母版链路）"""
    if not entry:
        return None
    _sldid, rid = entry
    m = re.search(r'<Relationship Id="%s"[^>]*Target="([^"]+)"[^>]*/>' % rid, pres_rels)
    if not m:
        return None
    part = m.group(1).lstrip("/")
    if not part.startswith("ppt/"):
        part = "ppt/" + part
    if part not in pkg:
        return None
    head, tail = part.rsplit("/", 1)
    rels = pkg.get(head + "/_rels/" + tail + ".rels", b"").decode("utf-8", "replace")
    return pkg[part].decode("utf-8"), rels


def _tpl_content_slide(sample, title, imgs, media, grid_cols, image_per_chart,
                       canvas=TPL_SLD_SZ):
    """内容页统一入口：有样例页 → 克隆（R13，装饰保留）；样例页缺失
    （纯封面模板）→ 内置几何原路径（R08 实现；R15 起几何按模板画布缩放，
    母版画布=原常量逐位相等）"""
    if sample is not None:
        return tpl_clone_slide(sample[0], sample[1], title, imgs, media,
                               grid_cols, image_per_chart, canvas)
    if image_per_chart:
        return tpl_grid_slide(title, imgs, media, grid_cols, canvas)
    return tpl_pageimage_slide(title, imgs[0], media, canvas)


def build_pptx_from_template(ctx, output_path, template_path):
    """模板化导出：模板页1（封面）原样保留，删除其余全部幻灯片，
    按上下文逐节（视图页）追加内容页；内容页 20261103-R13 起克隆模板
    内容页样例页（页2=勾选态/页3=不勾选态，保留装饰只换标题与图片区），
    无样例页回退内置几何；同步 presentation.xml/rels 与
    [Content_Types].xml。sections/items 形状与旧契约一致。"""
    pkg = read_package(template_path)
    if "ppt/presentation.xml" not in pkg or "ppt/_rels/presentation.xml.rels" not in pkg:
        raise ValueError("not a valid pptx package: " + template_path)

    pres = pkg["ppt/presentation.xml"].decode("utf-8")
    pres_rels = pkg["ppt/_rels/presentation.xml.rels"].decode("utf-8")
    content_types = pkg["[Content_Types].xml"].decode("utf-8")

    # 模板画布（R15）：内置几何常量按母版画布量取,内容页兜底插入框/标准
    # 标题带按模板 sldSz 逐轴等比缩放（母版画布=原值逐位相等）。sldSz 缺失
    # /解析失败/非法值回退母版画布（sldSz 随包整体保留的既有行为不变）
    sld_m = re.search(r'<p:sldSz[^>]*\scx="(\d+)"[^>]*\scy="(\d+)"', pres)
    canvas = (int(sld_m.group(1)), int(sld_m.group(2))) if sld_m else TPL_SLD_SZ
    if canvas[0] <= 0 or canvas[1] <= 0:
        canvas = TPL_SLD_SZ

    # sldIdLst：首项（封面）保留，其余删除
    lst_m = re.search(r"<p:sldIdLst>(.*?)</p:sldIdLst>", pres, re.S)
    if not lst_m:
        raise ValueError("template presentation.xml has no sldIdLst")
    entries = re.findall(r'<p:sldId id="(\d+)" r:id="(rId\d+)"/>', lst_m.group(1))
    if not entries:
        raise ValueError("template has no slides")
    kept = entries[0]
    dropped = entries[1:]

    # 样例页定位（R13 内容页克隆）：sldIdLst 序页2=勾选态（宫格）样例、
    # 页3=不勾选态（整页）样例；只存在其一则两态共用；都缺（纯封面模板）
    # → None，内容页回退内置几何原路径。须在下方删除循环弹出样例页部件
    # 之前提取（克隆基底 = 样例页 XML 一份处理后克隆）
    grid_sample = _tpl_sample_page(pkg, pres_rels, dropped[0] if dropped else None)
    page_sample = _tpl_sample_page(pkg, pres_rels, dropped[1] if len(dropped) > 1 else None)
    if page_sample is None:
        page_sample = grid_sample

    # 删除被删 slide 的包条目/关系/内容类型登记（其引用的 media/tags 留包不碍）
    for _sid, rid in dropped:
        m = re.search(r'<Relationship Id="%s"[^>]*Target="([^"]+)"[^>]*/>' % rid, pres_rels)
        if not m:
            continue
        target = m.group(1)
        part = target.lstrip("/")
        if not part.startswith("ppt/"):
            part = "ppt/" + part
        pkg.pop(part, None)
        head, tail = part.rsplit("/", 1)
        pkg.pop(head + "/_rels/" + tail + ".rels", None)
        content_types = re.sub(r'<Override PartName="/%s"[^>]*/>' % re.escape(part),
                               "", content_types)
    for _sid, rid in dropped:
        pres_rels = re.sub(r'<Relationship Id="%s"[^>]*/>' % re.escape(rid), "", pres_rels)

    # 新页编号/关系 id 接在模板现存最大值之后，避免撞名
    max_slide_no = 0
    for name in pkg:
        m = re.match(r"ppt/slides/slide(\d+)\.xml$", name)
        if m:
            max_slide_no = max(max_slide_no, int(m.group(1)))
    max_rid = max(int(x) for x in re.findall(r'Id="rId(\d+)"', pres_rels))
    max_sldid = max(int(e[0]) for e in entries)

    # 内容包缺失 png Default 时补登记（新页图片为 PNG）
    if 'Extension="png"' not in content_types:
        content_types = content_types.replace(
            "</Types>", '<Default Extension="png" ContentType="image/png"/></Types>')

    # 逐节生成内容页：勾选=每图窗一图，列数=节级布局 gridCols（0=自动 3 列）、
    # 行数自适应，同节全部图窗排在同一张幻灯片（20261102-R06 取消超 6 图续页，
    # 20261102-R07 跟随页面布局）；不勾=每节单张整页大图。有样例页走克隆
    # （20261103-R13 装饰保留），无样例页走内置几何（R08 原路径）
    media = TemplateMediaNamer(pkg.keys())
    image_per_chart = bool(ctx.get("imagePerChart", False))
    new_entries = []
    for section in ctx.get("sections", []):
        title = section.get("title", "")
        grid_cols = int(section.get("gridCols", 0) or 0)
        imgs = [it for it in section.get("items", [])
                if it.get("type", 0) == 1 and os.path.exists(it.get("imagePath", ""))]
        pages = []
        if image_per_chart:
            if imgs:
                pages.append(_tpl_content_slide(grid_sample, title, imgs, media,
                                                grid_cols, True, canvas))
        else:
            for item in imgs:
                pages.append(_tpl_content_slide(page_sample, title, [item], media,
                                                0, False, canvas))
        for slide_xml_str, rels_xml_str in pages:
            no = max_slide_no + 1 + len(new_entries)
            part = "ppt/slides/slide%d.xml" % no
            rels_part = "ppt/slides/_rels/slide%d.xml.rels" % no
            pkg[part] = slide_xml_str.encode("utf-8")
            pkg[rels_part] = rels_xml_str.encode("utf-8")
            content_types = content_types.replace(
                "</Types>",
                '<Override PartName="/%s" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/></Types>' % part)
            rid = "rId%d" % (max_rid + 1 + len(new_entries))
            pres_rels = pres_rels.replace(
                "</Relationships>",
                '<Relationship Id="%s" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide" Target="slides/slide%d.xml"/></Relationships>' % (rid, no))
            new_entries.append((max_sldid + 1 + len(new_entries), rid))

    # 重写 sldIdLst：封面 + 新内容页（保留模板 presentation.xml 其余内容，
    # 含模板 sldSz/notesSz）
    sld_ids = '<p:sldId id="%s" r:id="%s"/>' % kept
    for sid, rid in new_entries:
        sld_ids += '<p:sldId id="%d" r:id="%s"/>' % (sid, rid)
    pres = pres[:lst_m.start()] + "<p:sldIdLst>%s</p:sldIdLst>" % sld_ids + pres[lst_m.end():]

    pkg["ppt/presentation.xml"] = pres.encode("utf-8")
    pkg["ppt/_rels/presentation.xml.rels"] = pres_rels.encode("utf-8")
    pkg["[Content_Types].xml"] = content_types.encode("utf-8")

    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as z:
        for name, data in pkg.items():
            z.writestr(name, data)
        # 新增媒体全局唯一，收尾统一写一次
        for src, media_name in media.used.items():
            z.write(src, "ppt/media/" + media_name)


def build_pptx(json_path, output_path, logo_path="", template_path=""):
    with open(json_path, "r", encoding="utf-8") as f:
        ctx = json.load(f)

    # 模板化分支（曲线报告导出；templateMode 缺省/false 时走下方旧逻辑，
    # 行为与旧版逐字节级等价）。模板路径优先取 argv（C++ 侧已解析为真实
    # 磁盘文件：自定义=用户路径，默认=临时目录释放副本），回退 JSON 键。
    if ctx.get("templateMode"):
        tpl = template_path or ctx.get("templatePath") or ""
        if not tpl or not os.path.exists(tpl):
            raise ValueError("templateMode on but template file missing: %r" % tpl)
        build_pptx_from_template(ctx, output_path, tpl)
        return

    meta = ctx.get("meta", {})
    sections = ctx.get("sections", [])
    # 版式开关：false=每节图片合并为一页网格（导出弹窗"每个窗口导出至单独
    # 幻灯片"未勾选）；缺省 true 保持每图一页的既有行为
    window_per_slide = ctx.get("windowPerSlide", True)

    b = PptxBuilder(logo_path)
    add_title_slide(b, meta)

    for section in sections:
        add_section_slide(b, section)
        section_title = section.get("title", "")
        items = section.get("items", [])
        for item in items:
            t = item.get("type", 0)
            if t == 0:
                add_text_slide(b, item, section_title)
            elif t == 2:
                add_table_slide(b, item, section_title)
        if not window_per_slide:
            add_combined_images_slide(b, items, section_title)
        else:
            for item in items:
                if item.get("type", 0) == 1:
                    add_image_slide(b, item, section_title)

    # 报告末尾追加动画页（支持多个）
    animations = ctx.get("animations") or []
    if not animations and ctx.get("animation"):
        animations = [ctx["animation"]]
    for anim in animations:
        add_animation_slide(b, anim)

    b.save(output_path)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.stderr.write("usage: export_pptx.py <ctx.json> <output.pptx> [logo.png] [template.pptx]\n")
        sys.exit(1)
    try:
        logo = sys.argv[3] if len(sys.argv) > 3 else ""
        template = sys.argv[4] if len(sys.argv) > 4 else ""
        build_pptx(sys.argv[1], sys.argv[2], logo, template)
    except Exception as e:
        sys.stderr.write("pptx export failed: %s\n" % e)
        sys.exit(1)
