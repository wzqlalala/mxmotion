# ============================================================
# 接触二次开发类（JSON全字符串传参，经 executeAPIJson 调 C++ contact API）
# 用法：
#   Contact("C1", type="面面接触").set_master_geo_faces("1,2,3").set_slave_geo_faces("4,5").set_small_slip(True).create()
#   Contact("C1").set_type("自接触").set_small_slip(False).update()
# ============================================================
import json
from ctypes import *


class Contact:
    def __init__(self, name, type="面面接触"):
        # 所有值以字符串形式存储，由 C++ 侧统一解析转换
        self._data = {"name": str(name), "type": str(type)}

    # ============ 主面/从面：按类型单独封装 ============
    # 格式说明：id 支持扩展字段 "id@part_id:id2"（单元/单元表面等）
    #   - GeoFace/GeoPoint/GeoEdge/GeoSolid/Node：只需 id
    #   - Element/ElementFace/ElementEdge/Part：可带 part_id 和 id2，如 "10@1:0"

    def set_master_geo_faces(self, ids):
        # 主面-几何面，如 "1,2,3" 或 ["1","2","3"]
        self._append_pick_ids("master", "GeoFace", ids)
        return self

    def set_master_geo_points(self, ids):
        # 主面-几何点
        self._append_pick_ids("master", "GeoPoint", ids)
        return self

    def set_master_geo_edges(self, ids):
        # 主面-几何边
        self._append_pick_ids("master", "GeoEdge", ids)
        return self

    def set_master_geo_solids(self, ids):
        # 主面-几何体
        self._append_pick_ids("master", "GeoSolid", ids)
        return self

    def set_master_nodes(self, ids):
        # 主面-节点，如 "5,6,7"
        self._append_pick_ids("master", "Node", ids)
        return self

    def set_master_elements(self, ids):
        # 主面-单元，如 "10,11"（无 part_id 时自动查找）或 "10@1:0,11@1:0"
        self._append_pick_ids("master", "Element", ids)
        return self

    def set_master_element_faces(self, ids):
        # 主面-单元表面，必须带 part_id 和 id2，如 "10@1:0,11@1:0"
        self._append_pick_ids("master", "ElementFace", ids)
        return self

    def set_master_element_edges(self, ids):
        # 主面-单元边，必须带 part_id 和 id2，如 "10@1:0,11@1:0"
        self._append_pick_ids("master", "ElementEdge", ids)
        return self

    def set_master_parts(self, ids):
        # 主面-部件
        self._append_pick_ids("master", "Part", ids)
        return self

    def set_slave_geo_faces(self, ids):
        # 从面-几何面
        self._append_pick_ids("slave", "GeoFace", ids)
        return self

    def set_slave_geo_points(self, ids):
        # 从面-几何点
        self._append_pick_ids("slave", "GeoPoint", ids)
        return self

    def set_slave_geo_edges(self, ids):
        # 从面-几何边
        self._append_pick_ids("slave", "GeoEdge", ids)
        return self

    def set_slave_geo_solids(self, ids):
        # 从面-几何体
        self._append_pick_ids("slave", "GeoSolid", ids)
        return self

    def set_slave_nodes(self, ids):
        # 从面-节点
        self._append_pick_ids("slave", "Node", ids)
        return self

    def set_slave_elements(self, ids):
        # 从面-单元，如 "10,11"（无 part_id 时自动查找）或 "10@1:0,11@1:0"
        self._append_pick_ids("slave", "Element", ids)
        return self

    def set_slave_element_faces(self, ids):
        # 从面-单元表面，必须带 part_id 和 id2，如 "10@1:0,11@1:0"
        self._append_pick_ids("slave", "ElementFace", ids)
        return self

    def set_slave_element_edges(self, ids):
        # 从面-单元边，必须带 part_id 和 id2，如 "10@1:0,11@1:0"
        self._append_pick_ids("slave", "ElementEdge", ids)
        return self

    def set_slave_parts(self, ids):
        # 从面-部件
        self._append_pick_ids("slave", "Part", ids)
        return self

    def _append_pick_ids(self, key, pick_type, ids):
        """把指定类型的ID设置到 master/slave 字符串中（同类型覆盖）
        格式："GeoFace:1,2,3;Node:5,6"
        """
        # 统一转为字符串列表
        if isinstance(ids, (list, tuple)):
            id_list = [str(i) for i in ids]
        else:
            id_list = [str(ids)]

        # 组装 "Type:id1,id2"
        group_str = pick_type + ":" + ",".join(id_list)

        existing = self._data.get(key, "")
        if not existing:
            self._data[key] = group_str
            return

        # 按分号拆分，查找并替换同类型组
        prefix = pick_type + ":"
        groups = existing.split(";")
        for i, group in enumerate(groups):
            if group.startswith(prefix):
                groups[i] = group_str  # 覆盖同类型
                self._data[key] = ";".join(groups)
                return

        # 没有该类型，追加新组
        self._data[key] = existing + ";" + group_str

    def set_master(self, ids):
        # 兼容旧接口：直接传入完整格式字符串 "GeoFace:1,2,3;Node:5,6"
        self._data["master"] = str(ids)
        return self

    def set_slave(self, ids):
        # 兼容旧接口：直接传入完整格式字符串 "GeoFace:3,4;Node:7"
        self._data["slave"] = str(ids)
        return self

    def set_master_set(self, set_name):
        # 主面直接引用已有集合名称（与 set_master 互斥）
        self._data["masterSet"] = str(set_name)
        return self

    def set_slave_set(self, set_name):
        # 从面直接引用已有集合名称（与 set_slave 互斥）
        self._data["slaveSet"] = str(set_name)
        return self

    def set_contact_pair(self, contact_name):
        # 过盈接触-作用对象：引用未抑制的面面/点面/二维接触名称
        self._data["contactPair"] = str(contact_name)
        return self

    def set_function(self, function_name):
        # 过盈接触-函数映射：引用已有函数名称
        self._data["function"] = str(function_name)
        return self

    def set_analysis_step(self, case_name):
        # 过盈接触-分析步映射：引用已有分析步(工况)名称
        self._data["analysisStep"] = str(case_name)
        return self

    def remove_analysis_step(self, case_name):
        # 解除过盈接触与指定分析步的映射
        self._data["analysisStepRemove"] = str(case_name)
        return self

    def swap(self):
        # 交换主/从面映射
        self._data["swap"] = "true"
        return self

    def set_type(self, type):
        # 切换接触类型：面面接触/点面接触/通用接触/自接触/过盈接触/二维接触/线面接触
        self._data["type"] = str(type)
        return self

    def set_master_dir(self, on):
        self._data["masterDir"] = "true" if on else "false"
        return self

    def set_slave_dir(self, on):
        self._data["slaveDir"] = "true" if on else "false"
        return self

    def set_slave_region(self, region):
        # 点面/线面接触从面区域：0-单元面，1-节点表面
        self._data["slaveRegion"] = str(region)
        return self

    def set_small_slip(self, on):
        self._data["smallSlip"] = "true" if on else "false"
        return self

    def set_self_contact_search(self, on):
        self._data["selfContactSearch"] = "true" if on else "false"
        return self

    def set_pressure_overclosure(self, v):
        # 压力过盈：硬接触/完全绑定接触/法向不分离接触/线性接触（二维接触仅硬接触/线性接触）
        self._data["overclosure"] = str(v)
        return self

    def set_constraint(self, v):
        # 约束方法：罚函数法/增广-摄动拉格朗日乘子法/多点约束法
        self._data["constraint"] = str(v)
        return self

    def set_scale_factor(self, v):
        self._data["scaleFactor"] = str(v)
        return self

    def set_penalty_factor(self, v):
        self._data["penaltyFactor"] = str(v)
        return self

    def set_tangential(self, define, friction="无摩擦", coef="0.0"):
        # 切向行为：define-开启/关闭，friction-无摩擦/通用接触摩擦/粗糙（二维接触无粗糙），coef-静摩擦系数
        self._data["tangentialDefine"] = "true" if define else "false"
        self._data["tangentialFriction"] = str(friction)
        self._data["tangentialStaticCoef"] = str(coef)
        return self

    def set_heat_conductance(self, define, table=None):
        # 热接触间隙传导：table 为 ["导热系数:间隙值", ...]，如 ["10.5:0.1","20.3:0.2"]
        self._data["heatDefine"] = "true" if define else "false"
        if table is not None:
            self._data["heatValue"] = [str(item) for item in table]
        return self

    def set_gap(self, define, overclosure="0.0"):
        # 接触间隙：define-开启/关闭，overclosure-过盈量
        self._data["gapDefine"] = "true" if define else "false"
        self._data["gapOverclosure"] = str(overclosure)
        return self

    def set_geom_adjust(self, define, value="0.0"):
        # 几何调整：define-开启/关闭，value-调整值
        self._data["geomDefine"] = "true" if define else "false"
        self._data["geomValue"] = str(value)
        return self

    def set_clearance_fit(self, typ, size=""):
        # 间隙配合：typ-0零间隙配合/1逐步消除间隙配合量，size-大小
        self._data["clrFitType"] = str(typ)
        self._data["size"] = str(size)
        return self

    def set_evaluation(self, on, mp=False, sd=False, cv=False, pc=False, r=False):
        # 接触压力均匀性评估指标
        self._data["evaluation"] = "true" if on else "false"
        self._data["evaMP"] = "true" if mp else "false"
        self._data["evaSD"] = "true" if sd else "false"
        self._data["evaCV"] = "true" if cv else "false"
        self._data["evaPC"] = "true" if pc else "false"
        self._data["evaR"] = "true" if r else "false"
        return self

    def create(self):
        self._data["isCreate"] = "true"
        _mw.executeAPIJson(b"contact", json.dumps(self._data, ensure_ascii=False).encode("utf-8"))

    def update(self):
        self._data["isCreate"] = "false"
        _mw.executeAPIJson(b"contact", json.dumps(self._data, ensure_ascii=False).encode("utf-8"))


# 由 CommandMech.py 在加载时注入
_mw = None


def _set_mw(mw):
    """设置 ctypes 库句柄（由 CommandMech.py 调用）"""
    global _mw
    _mw = mw
