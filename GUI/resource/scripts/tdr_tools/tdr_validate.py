#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""tdr_validate.py——容器校验工具（FR-13 三层校验独立命令）。

用法：<python> tdr_validate.py <container.rtdoc> [dataset.json]
  第一层 容器完整性：manifest SHA1 逐部件对账、格式版本、去向对账表完备性；
  第二层 结构与定位：规则语法/必填/枚举；每槽声明经锚点+occurrence 唯一解析
         （校验器不保证语义正确性——D-18 人工检查单把关）；
  第三层 引用一致性：数据键在数据字典有定义；高亮全覆盖契约核对。
输出：逐条"文件+规则条目+原因"（FR-13 验收口径）；退出码 0=通过。
与引擎 M1 同套逻辑（复用 tdrrules/tdroox）；入口 sys.path 注入
<脚本根>/../tdr_engine（B5/T-02-a 两形态同构）。
"""

import io
import json
import math
import os
import sys
from typing import Any, Dict, List, Optional

sys.path.insert(0, os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "tdr_engine")))

import tdroox   # noqa: E402
import tdrrules  # noqa: E402
from tdr_engine import EngineError, load_and_verify_container  # noqa: E402


def _reject_nonfinite_constant(name):
    # type: (str) -> float
    """CC-12 同口径：数据集 NaN/Infinity 字面=非法（校验即错，与引擎一致）。
    DX-11：措辞与引擎统一（含改写建议）"""
    raise ValueError(u"数据集含非有限数值常量 %r（NaN/Infinity 不落文档，"
                     u"请改写为 null 或合法数值）" % name)


def _finite_float(text):
    # type: (str) -> float
    v = float(text)
    if not math.isfinite(v):
        raise ValueError(u"数据集含非有限数值 %s（NaN/Infinity 不落文档）" % text)
    return v


def main(argv):
    # type: (List[str]) -> int
    if len(argv) < 1:
        sys.stderr.write(u"usage: tdr_validate.py <container.rtdoc> [dataset.json]\n"
                         u"退出码：0=通过，1=失败（详见输出），2=用法错误\n")
        return 2
    container_path = argv[0]
    dataset = None   # type: Optional[Dict[str, Any]]
    if len(argv) > 1:
        try:
            with io.open(argv[1], "r", encoding="utf-8") as f:
                loaded = json.load(f, parse_constant=_reject_nonfinite_constant,
                                   parse_float=_finite_float)
            if isinstance(loaded, dict):
                dataset = loaded
            else:
                # DX-12：显式给了数据集路径却未生效时带指引
                sys.stderr.write(u"[validate] 数据集顶层不是 JSON 对象"
                                 u"（仅影响实例键存在性探测，跳过该探测；"
                                 u"如需键存在性探测请检查数据集内容）\n")
                dataset = None
        except ValueError as e:
            # CC-12：NaN/Infinity 数据集=校验失败（与引擎 M1 同判定，分类对称）
            sys.stdout.write(u"[validate] 校验失败：数据集非法：%s\n" % e)
            return 1
        except Exception as e:
            sys.stderr.write(u"[validate] 数据集读取失败（仅影响实例键存在性探测，"
                             u"如需键存在性探测请检查路径）：%s\n" % e)
            dataset = None
    # CA-06：无数据集参数=传 None 跳过 instance_key 存在性探测（与 docstring
    # "dataset 可选"口径一致；旧实现传 {} 恒误报"数据集缺少实例数组 'cases'"）
    try:
        con = load_and_verify_container(container_path, dataset)
    except EngineError as e:
        sys.stdout.write(u"[validate] 校验失败：%s\n" % e.msg)
        for er in e.errors:
            sys.stdout.write(u"  - %s\n" % er)
        return 1
    warns = tdrrules.validate_rules(con.rules, con.index, dataset)
    warn_items = [e for e in warns if e.is_warn]
    sys.stdout.write(u"[validate] 通过：容器 %s\n" % container_path)
    sys.stdout.write(u"[validate] 模板=%s | 部件=%d | 块=%d | 槽=%d\n"
                     % (con.manifest.get("template_id"),
                        len(con.manifest.get("parts", [])),
                        con.manifest.get("blocks", {}).get("count", 0),
                        len(con.rules.get("slots", []))))
    for w in warn_items:
        sys.stdout.write(u"[validate] warn：%s\n" % w)
    return 0


if __name__ == "__main__":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8")
    sys.exit(main(sys.argv[1:]))
