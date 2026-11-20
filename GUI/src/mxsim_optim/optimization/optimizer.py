# -*- coding: utf-8 -*-
"""
优化器工厂 + 问题规格描述。

核心类：
    ObjectiveSpec     — 单个目标的描述（最小化/最大化）
    ConstraintSpec    — 单个约束的描述（<=, >=, ==）
    ProblemSpec       — 完整优化问题的描述
    OptimizerFactory  — 统一创建/恢复优化器

ProblemSpec 使得 C++ 调用者无需手动处理目标方向或约束形式转换：
    - 最大化目标 → 内部自动取反为最小化
    - g>=a 约束 → 内部自动转为 a-g<=0
    - g==a 约束 → 内部展开为两个不等约束

用法::

    spec = ProblemSpec(
        objectives=[ObjectiveSpec("maximize"), ObjectiveSpec("minimize")],
        constraints=[ConstraintSpec(">=", bound=0.5)],
    )
    opt = OptimizerFactory.create("PSO", ranges, hyperparams, spec)
    pop = opt.initialize()
    # C++ CAE 直接传入**原始**目标值和约束值：
    opt.update(raw_f, raw_g)   # 转换在内部自动完成
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from mxsim_optim.base import AlgorithmRegistry, MxsimValidationError


# =============================================================================
# 问题描述数据结构
# =============================================================================

@dataclass
class ObjectiveSpec:
    """单个优化目标的描述。

    Parameters
    ----------
    direction : str
        ``"minimize"`` 或 ``"maximize"``。
    name : str, optional
        目标名称（用于结果展示）。
    """
    direction: str = "minimize"
    name: str = ""

    def __post_init__(self):
        if self.direction not in ("minimize", "maximize"):
            raise MxsimValidationError(
                f"目标方向必须是 'minimize' 或 'maximize'，"
                f"得到 '{self.direction}'")

    @property
    def is_maximize(self) -> bool:
        return self.direction == "maximize"


@dataclass
class ConstraintSpec:
    """单个约束的描述。

    Parameters
    ----------
    type : str
        ``"<="``、``">="`` 或 ``"=="``。
    bound : float
        约束的右端常数：
        - ``"<="`` → g(x) <= bound
        - ``">="`` → g(x) >= bound
        - ``"=="`` → g(x) == bound
    name : str, optional
        约束名称（用于结果展示）。
    """
    type: str = "<="
    bound: float = 0.0
    name: str = ""

    def __post_init__(self):
        if self.type not in ("<=", ">=", "=="):
            raise MxsimValidationError(
                f"约束类型必须是 '<=', '>=', 或 '=='，得到 '{self.type}'")


@dataclass
class ProblemSpec:
    """优化问题的完整描述。

    用户在此声明目标方向与约束形式，优化器内部自动转换为
    标准形式（全部最小化、全部 g(x) <= 0）。

    快捷写法::

        ProblemSpec(n_objectives=2)                    # 纯最小化，无约束
        ProblemSpec(objectives=["maximize"])           # 最大化
        ProblemSpec(objectives=["minimize"],
                     constraints=[ConstraintSpec(">=", bound=1.0)])
    """
    objectives: List[ObjectiveSpec] = field(default_factory=list)
    constraints: List[ConstraintSpec] = field(default_factory=list)

    def __post_init__(self):
        # 支持 ProblemSpec(n_objectives=2) 的便捷写法
        if isinstance(self.objectives, int):
            self.objectives = [ObjectiveSpec("minimize")
                               for _ in range(self.objectives)]
        # 支持 objectives=["maximize"] 的字符串写法
        self.objectives = [
            ObjectiveSpec(d) if isinstance(d, str) else d
            for d in self.objectives
        ]
        self.constraints = [
            ConstraintSpec(c["type"], c.get("bound", 0))
            if isinstance(c, dict) else c
            for c in self.constraints
        ]

    # ---- 属性 ----

    @property
    def n_objectives(self) -> int:
        return len(self.objectives)

    @property
    def n_constraints_original(self) -> int:
        """原始约束个数（== 约束会在内部展开为 2 个）。"""
        return len(self.constraints)

    @property
    def n_constraints_internal(self) -> int:
        """内部标准形式的约束个数。"""
        return sum(2 if c.type == "==" else 1 for c in self.constraints)

    @property
    def has_maximization(self) -> bool:
        return any(o.is_maximize for o in self.objectives)

    # ---- 转换方法 ----

    def transform_objectives(self, F_raw: np.ndarray) -> np.ndarray:
        """将原始目标值转换为标准形式（全部最小化）。

        maximize f → -f，minimize f → 不变。

        Parameters
        ----------
        F_raw : shape (N, n_obj)
            C++ CAE 计算出的原始目标值。

        Returns
        -------
        shape (N, n_obj)，全部为最小化方向。
        """
        F = np.atleast_2d(F_raw).astype(np.float64)
        if F.shape[0] == 1 and F.shape[1] != self.n_objectives:
            F = F.T
        for k, obj in enumerate(self.objectives):
            if obj.is_maximize:
                F[:, k] = -F[:, k]
        return F

    def transform_constraints(self, G_raw: Optional[np.ndarray]) -> np.ndarray:
        """将原始约束值转换为标准形式（全部 g <= 0）。

        g >= bound  →  bound - g <= 0
        g <= bound  →  g - bound <= 0
        g == bound  →  [g-bound, bound-g] <= 0

        Parameters
        ----------
        G_raw : shape (N, n_con_original) 或 None

        Returns
        -------
        shape (N, n_con_internal)
        """
        if G_raw is None or len(self.constraints) == 0:
            return np.zeros((1, 0))

        G = np.atleast_2d(G_raw).astype(np.float64)
        if G.shape[0] == 1 and G.shape[1] != self.n_constraints_original:
            G = G.T

        cols = []
        for k, con in enumerate(self.constraints):
            g = G[:, k]
            if con.type == "<=":
                cols.append(g - con.bound)
            elif con.type == ">=":
                cols.append(con.bound - g)
            elif con.type == "==":
                cols.append(g - con.bound)
                cols.append(con.bound - g)

        return np.column_stack(cols) if cols else np.zeros((G.shape[0], 1))

    # ---- 反向转换（用于 get_history 还原原始值） ----

    def inverse_objectives(self, F_int: np.ndarray) -> np.ndarray:
        """将内部目标值还原为原始目标值（maximize 的取反还原）。"""
        F = np.atleast_2d(F_int).astype(np.float64).copy()
        for k, obj in enumerate(self.objectives):
            if obj.is_maximize:
                F[:, k] = -F[:, k]
        return F

    def inverse_constraints(self, G_int: Optional[np.ndarray]) -> Optional[np.ndarray]:
        """将内部约束值还原为原始约束值。"""
        if G_int is None or len(self.constraints) == 0:
            return None
        G = np.atleast_2d(G_int).astype(np.float64)
        cols = []
        idx = 0
        for con in self.constraints:
            if con.type == "==":
                # 内部展开为两列 [g-bound, bound-g]，还原取第一列
                cols.append(G[:, idx] + con.bound)
                idx += 2
            elif con.type == ">=":
                cols.append(con.bound - G[:, idx])
                idx += 1
            else:  # "<="
                cols.append(G[:, idx] + con.bound)
                idx += 1
        return np.column_stack(cols)

    # ---- 序列化 ----

    def to_dict(self) -> Dict[str, Any]:
        """序列化为可 JSON 传输的字典。"""
        return {
            "objectives": [{"direction": o.direction, "name": o.name}
                           for o in self.objectives],
            "constraints": [{"type": c.type, "bound": c.bound, "name": c.name}
                            for c in self.constraints],
        }

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "ProblemSpec":
        return cls(
            objectives=[ObjectiveSpec(**o) if isinstance(o, dict) else o
                        for o in d.get("objectives", [])],
            constraints=[ConstraintSpec(**c) if isinstance(c, dict) else c
                         for c in d.get("constraints", [])],
        )

    @classmethod
    def default(cls, n_objectives: int = 1, n_constraints: int = 0) -> "ProblemSpec":
        """创建默认问题描述（全部最小化，约束 g<=0）。"""
        return cls(
            objectives=[ObjectiveSpec("minimize") for _ in range(n_objectives)],
            constraints=[ConstraintSpec("<=", bound=0.0) for _ in range(n_constraints)],
        )


# =============================================================================
# 带问题转换的包装器
# =============================================================================

class _OptimizerWithProblemSpec:
    """在内层优化器外包装一层目标/约束转换。

    对外接口与普通优化器完全一致（initialize / update / is_converged / ...），
    但在 update 时自动完成 max→min 取反和约束规范化。
    """

    def __init__(self, raw_optimizer, problem_spec: ProblemSpec):
        self._opt = raw_optimizer
        self._spec = problem_spec

    @property
    def design_ranges(self): return self._opt.design_ranges
    @property
    def n_vars(self): return self._opt.n_vars
    @property
    def n_objectives(self): return self._spec.n_objectives
    @property
    def n_constraints(self): return self._spec.n_constraints_original
    @property
    def problem_spec(self) -> ProblemSpec: return self._spec

    def initialize(self) -> np.ndarray:
        return self._opt.initialize()

    def update(
        self,
        objective_values: np.ndarray,
        constraint_values: Optional[np.ndarray] = None,
    ) -> np.ndarray:
        """C++ 传入原始目标/约束，内部自动转换后传给优化器。"""
        F_int = self._spec.transform_objectives(objective_values)
        G_int = self._spec.transform_constraints(constraint_values)

        if self._spec.n_constraints_internal == 0:
            return self._opt.update(F_int, None)
        return self._opt.update(F_int, G_int)

    def is_converged(self) -> bool:
        return self._opt.is_converged()

    def get_history(self) -> List[Dict[str, Any]]:
        """返回恢复为原始目标/约束形式的迭代历史。"""
        raw = self._opt.get_history()
        if not raw:
            return raw
        # 对每条历史记录做反向转换
        result = []
        for entry in raw:
            e = dict(entry)
            # 还原目标
            F_int = np.array(e.get("objectives", []), dtype=np.float64)
            if F_int.size > 0:
                e["objectives"] = self._spec.inverse_objectives(F_int).tolist()
            # 还原约束
            G_int = e.get("constraints")
            if G_int is not None:
                G_arr = np.array(G_int, dtype=np.float64)
                if G_arr.size > 0:
                    G_orig = self._spec.inverse_constraints(G_arr)
                    e["constraints"] = G_orig.tolist() if G_orig is not None else None
            result.append(e)
        return result

    def get_state(self) -> Dict[str, Any]:
        state = self._opt.get_state()
        state["problem_spec"] = self._spec.to_dict()
        return state

    def load_state(self, state: Dict[str, Any]) -> None:
        self._opt.load_state(state)


# =============================================================================
# 优化器工厂
# =============================================================================

class OptimizerFactory:
    """优化器统一创建入口。

    用法::

        # 最简单
        opt = OptimizerFactory.create('PSO', ranges=[[0,1], [0,1]])

        # 带问题描述
        spec = ProblemSpec(objectives=[ObjectiveSpec("maximize")])
        opt = OptimizerFactory.create('NSGA2', ranges, hyperparams, spec)

        # 从保存的状态恢复
        opt = OptimizerFactory.create_from_state(saved_state)
    """

    @staticmethod
    def create(
        algorithm: str,
        design_ranges: List[Tuple[float, float]],
        hyperparams: Optional[Dict[str, Any]] = None,
        problem_spec: Optional[ProblemSpec] = None,
    ):
        hyperparams = dict(hyperparams or {})

        # 根据 ProblemSpec 修正维度（强制使用 ProblemSpec 的值，避免不匹配）
        if problem_spec is not None:
            hyperparams["n_objectives"] = problem_spec.n_objectives
            hyperparams["n_constraints"] = problem_spec.n_constraints_internal

        raw = AlgorithmRegistry.create_optimizer(
            algorithm,
            design_ranges=design_ranges,
            hyperparams=hyperparams,
        )

        # 如果有个性化需求，包一层转换
        if problem_spec is not None and (
            problem_spec.has_maximization
            or problem_spec.n_constraints_original > 0
        ):
            return _OptimizerWithProblemSpec(raw, problem_spec)
        return raw

    @staticmethod
    def create_from_state(state: Dict[str, Any]):
        """从序列化状态恢复优化器（自动恢复 ProblemSpec）。"""
        algorithm = state.get("algorithm")
        if not algorithm:
            raise MxsimValidationError("状态字典缺少 'algorithm' 键。")
        design_ranges = state.get("design_ranges")
        if design_ranges is None:
            raise MxsimValidationError("状态字典缺少 'design_ranges' 键。")
        hyperparams = state.get("hyperparams", {})

        problem_spec = None
        if "problem_spec" in state:
            problem_spec = ProblemSpec.from_dict(state["problem_spec"])

        raw = AlgorithmRegistry.create_optimizer(
            algorithm,
            design_ranges=design_ranges,
            hyperparams=hyperparams,
        )
        raw.load_state(state)

        if problem_spec is not None and (
            problem_spec.has_maximization
            or problem_spec.n_constraints_original > 0
        ):
            return _OptimizerWithProblemSpec(raw, problem_spec)
        return raw
