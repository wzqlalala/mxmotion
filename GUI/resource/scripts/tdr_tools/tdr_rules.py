#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""tdr_rules.py——规则层取出/回填工具（T-10，兑现 02 §2.3 步骤 3 承诺）。

用法：
  <python> tdr_rules.py get <container.rtdoc> [out_rules.json]
      取出 rules/rules.json（剥除注释头后为合法 JSON，文本工具编辑；
      整行 # 注释约定见 03 §2.2）。
  <python> tdr_rules.py put <container.rtdoc> <rules.json>
      回填 rules 层：JSON 语法预检（load_rules schema 同套）→重写容器内
      rules/rules.json→自动重算 manifest 中 rules 相关完整性登记（sha1 无
      ——rules 层为人工编辑面、不进 SHA1 对账表；manifest 其余字段不动），
      并回写容器（zip 重打包，资产字节原样搬运）。
"""

import io
import json
import os
import sys
from typing import List

sys.path.insert(0, os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "tdr_engine")))

import tdrrules  # noqa: E402
import tdroox    # noqa: E402


def _load_rules_text(con):
    # type: (Dict[str, bytes]) -> str
    raw = con.get("rules/rules.json")
    if raw is None:
        raise ValueError(u"容器内缺少 rules/rules.json")
    return raw.decode("utf-8")


def _read_container(container):
    # type: (str) -> Dict[str, bytes]
    """DX-01：容器缺失/不可读/非 rtdoc 给可读文案（不再裸 traceback）。
    抛 _ContainerError（message 已含文件+原因+建议）。"""
    try:
        return tdroox.read_package(container)
    except OSError as e:
        raise _ContainerError(u"容器读取失败（文件不存在或不可读）：%s（%s）"
                              u"——请核对容器路径" % (container, e))
    except Exception as e:
        raise _ContainerError(u"容器读取失败（非 rtdoc 格式或已损坏，请重新提炼）：%s：%s"
                              % (container, e))


class _ContainerError(Exception):
    pass


def cmd_get(container, out_path):
    # type: (str, str) -> int
    try:
        con = _read_container(container)
    except _ContainerError as e:
        sys.stderr.write(u"[rules] %s\n" % e)
        return 1
    text = _load_rules_text(con)
    if not out_path:
        out_path = os.path.splitext(container)[0] + "_rules.json"
    with io.open(out_path, "w", encoding="utf-8") as f:
        f.write(text)
    sys.stdout.write(u"[rules] 已取出：%s\n" % out_path)
    return 0


def cmd_put(container, rules_path):
    # type: (str, str) -> int
    try:
        with io.open(rules_path, "r", encoding="utf-8") as f:
            text = f.read()
    except OSError as e:
        # DX-01 同族：规则文件缺失/不可读给可读文案（不再裸 traceback）
        sys.stderr.write(u"[rules] 规则文件读取失败（不存在或不可读）：%s（%s）"
                         u"——请核对规则文件路径\n" % (rules_path, e))
        return 1
    try:
        tdrrules.load_rules(text)   # JSON+schema 预检（不合格拒绝回填）
    except ValueError as e:
        # DX-13：JSON 语法类失败补规则文件名（原异常含行列号信息保留）
        msg = str(e)
        head = u"规则层 JSON 解析失败"
        if msg.startswith(head + u"："):
            msg = u"%s（%s：%s）" % (head, rules_path,
                                    msg[len(head) + 1:])
        sys.stderr.write(u"[rules] 回填拒绝（规则层非法）：%s\n" % msg)
        return 1
    try:
        con = _read_container(container)
    except _ContainerError as e:
        sys.stderr.write(u"[rules] %s\n" % e)
        return 1
    con["rules/rules.json"] = text.encode("utf-8")
    # CA-13：tmp 带 pid 后缀（无 pid 固定名在并发/崩溃残留时互相覆盖；GUI
    # 兜底清理前缀 <out>.tdr_tmp* 与本名不冲突，残留面登记于实施记录）
    tmp = container + ".tdr_rules_tmp%d" % os.getpid()
    try:
        tdroox.write_package(con, tmp)
        os.replace(tmp, container)
    except PermissionError:
        if os.path.exists(tmp):
            try:
                os.remove(tmp)
            except OSError:
                pass
        sys.stderr.write(u"[rules] 容器可能正被打开，请关闭后重试\n")
        return 1
    except OSError as e:
        # CA-13：非 PermissionError 的 OSError 给出可读文案（不再裸 traceback）
        if os.path.exists(tmp):
            try:
                os.remove(tmp)
            except OSError:
                pass
        sys.stderr.write(u"[rules] 回填失败（容器不可写或路径无效）：%s（%s）\n"
                         % (container, e))
        return 1
    sys.stdout.write(u"[rules] 已回填（manifest 其余登记不变）：%s\n" % container)
    return 0


def main(argv):
    # type: (List[str]) -> int
    if len(argv) >= 2 and argv[0] == "get":
        return cmd_get(argv[1], argv[2] if len(argv) > 2 else "")
    if len(argv) >= 3 and argv[0] == "put":
        return cmd_put(argv[1], argv[2])
    sys.stderr.write(u"usage: tdr_rules.py get <container.rtdoc> [out.json]\n"
                     u"       tdr_rules.py put <container.rtdoc> <rules.json>\n"
                     u"退出码：0=成功，1=失败（详见输出），2=用法错误\n")
    return 2


if __name__ == "__main__":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8")
    sys.exit(main(sys.argv[1:]))
