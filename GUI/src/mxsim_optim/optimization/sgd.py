# -*- coding: utf-8 -*-
"""
随机梯度下降（SGD）— 局部微调优化器。

采用中心有限差分估计梯度，支持动量加速。
约束处理：投影法（裁剪到可行域）或罚函数法。

适用于全局搜索后的局部精修阶段。
"""

from __future__ import annotations

from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from mxsim_optim.base import (
    OptimizerBase,
    MxsimValidationError,
    register_optimizer,
)
from mxsim_optim.utils import (
    unscale_from_unit,
    validate_positive_int,
    validate_positive_float,
)


@register_optimizer("SGD")
class SGD(OptimizerBase):
    """随机梯度下降（局部微调）。

    超参数
    ------
    learning_rate : float, default=0.01
        学习率（步长）。
    momentum : float, default=0.0
        动量系数（0=无动量）。
    n_iterations : int, default=50
        最大梯度步数。
    fd_eps : float, default=1e-6
        有限差分步长（相对于变量范围的比例）。
    constraint_method : str, default='projection'
        约束处理方法：``"projection"``（投影到可行域）或 ``"penalty"``（罚函数）。
    penalty_factor : float, default=1e4
        罚函数乘子（constraint_method='penalty' 时生效）。
    tol : float, default=1e-6
        梯度范数收敛容差。
    seed : int, optional
        随机种子。
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        super().__init__(design_ranges, hyperparams)

        self.learning_rate = self.hyperparams.get("learning_rate", 0.01)
        self.momentum = self.hyperparams.get("momentum", 0.0)
        self.n_iterations = self.hyperparams.get("n_iterations", 50)
        self.fd_eps = self.hyperparams.get("fd_eps", 1e-6)
        self.constraint_method = self.hyperparams.get("constraint_method", "projection")
        self.penalty_factor = self.hyperparams.get("penalty_factor", 1e4)
        self.tol = self.hyperparams.get("tol", 1e-6)
        self.seed = self.hyperparams.get("seed", None)

        validate_positive_float("learning_rate", self.learning_rate)
        validate_positive_int("n_iterations", self.n_iterations)
        validate_positive_float("fd_eps", self.fd_eps)
        if self.constraint_method not in ("projection", "penalty"):
            raise MxsimValidationError(
                f"constraint_method 必须是 'projection' 或 'penalty'，"
                f"得到 '{self.constraint_method}'。")

        self._disc = self.hyperparams.get("discrete_values", None)
        self._rng = np.random.RandomState(self.seed)
        self._x: Optional[np.ndarray] = None         # 当前点（单位空间）
        self._velocity: Optional[np.ndarray] = None  # 动量速度
        self._last_f: Optional[float] = None
        self._grad_norm: float = np.inf
        self._converged = False

    # ---- 辅助 ----

    def _to_phys(self, U: np.ndarray) -> np.ndarray:
        return unscale_from_unit(U, self.design_ranges)

    def _discretize(self, X):
        if self._disc is None: return X
        Xd = X.copy()
        for i, vals in enumerate(self._disc):
            if vals is not None:
                vv = np.asarray(vals).reshape(-1)
                lo, hi = self.design_ranges[i]
                vv_unit = (vv - lo) / (hi - lo + 1e-30)
                diffs = np.abs(Xd[:, i:i+1] - vv_unit.reshape(1, -1))
                Xd[:, i] = vv_unit[np.argmin(diffs, axis=1)]
        return Xd

    def _build_fd_batch(self) -> np.ndarray:
        """构建有限差分评估批次：中心点 + 2×n_vars 个扰动点。"""
        d = self.n_vars
        batch = np.zeros((1 + 2 * d, d))
        batch[0] = self._x
        eps = self.fd_eps
        for i in range(d):
            xp, xm = self._x.copy(), self._x.copy()
            xp[i] = min(1.0, self._x[i] + eps)
            xm[i] = max(0.0, self._x[i] - eps)
            batch[1 + 2*i] = xp
            batch[1 + 2*i + 1] = xm
        return batch

    # ---- 生命周期 ----

    def initialize(self) -> np.ndarray:
        """随机初始化，返回第一批评估点（中心点 + 差分扰动点）。"""
        self._x = self._rng.uniform(0.0, 1.0, size=self.n_vars)
        self._x = self._discretize(self._x.reshape(1, -1)).ravel()
        self._velocity = np.zeros(self.n_vars)
        self._last_f = None
        self._grad_norm = np.inf
        self._converged = False
        self._iteration = 0
        self._history = []
        return self._to_phys(self._build_fd_batch())

    def update(
        self,
        objective_values: np.ndarray,
        constraint_values: Optional[np.ndarray] = None,
    ) -> np.ndarray:
        """有限差分估计梯度 → 梯度下降一步。

        Parameters
        ----------
        objective_values : shape (1+2×n_vars,) 或 (1+2×n_vars, n_obj)
        constraint_values : shape (1+2×n_vars, n_con) 或 None
        """
        n_evals = 1 + 2 * self.n_vars
        F = np.asarray(objective_values, dtype=np.float64)
        if F.ndim == 1:
            F = F.reshape(-1, 1)  # SGD 始终单目标
        if F.shape[0] != n_evals:
            raise MxsimValidationError(
                f"SGD 期望 {n_evals} 个目标值（1 中心 + 2×{self.n_vars} 扰动），"
                f"收到 {F.shape[0]}。")

        self._iteration += 1
        d = self.n_vars

        f0 = float(F[0, 0]) if F.ndim > 1 else float(F[0])
        self._last_f = f0

        cv0 = 0.0
        if constraint_values is not None:
            cv0 = float(np.sum(np.maximum(0, constraint_values[0])**2))

        # 中心有限差分 → 单位空间梯度
        range_scale = self.design_ranges[:, 1] - self.design_ranges[:, 0]
        h = self.fd_eps * range_scale  # 物理空间扰动步长
        grad = np.zeros(d)
        for i in range(d):
            fp = float(F[1 + 2*i, 0]) if F.ndim > 1 else float(F[1 + 2*i])
            fm = float(F[1 + 2*i + 1, 0]) if F.ndim > 1 else float(F[1 + 2*i + 1])
            grad[i] = (fp - fm) / (2.0 * h[i]) * range_scale[i]

        # 罚函数梯度
        if self.constraint_method == "penalty" and constraint_values is not None:
            for i in range(d):
                cvp = float(np.sum(np.maximum(0, constraint_values[1 + 2*i])**2))
                cvm = float(np.sum(np.maximum(0, constraint_values[1 + 2*i + 1])**2))
                grad[i] += self.penalty_factor * (cvp - cvm) / (2.0 * h[i]) * range_scale[i]

        self._grad_norm = float(np.linalg.norm(grad))

        # 先存旧位置（匹配本次传入的评估值 f0）再更新
        old_x = self._x.tolist()

        # 动量 + 梯度步
        self._velocity = self.momentum * self._velocity - self.learning_rate * grad
        self._x = self._x + self._velocity
        self._x = np.clip(self._x, 0.0, 1.0)  # 投影
        self._x = self._discretize(self._x.reshape(1, -1)).ravel()

        if self._iteration >= self.n_iterations or self._grad_norm < self.tol:
            self._converged = True

        self._history.append({
            "iteration": self._iteration,
            "x": old_x,
            "objectives": [float(f0)],          # 使用 "objectives" key，方便 ProblemSpec 反向转换
            "grad_norm": float(self._grad_norm),
            "constraint_violation": cv0,
            "constraints": constraint_values[0].tolist()
                if constraint_values is not None and len(constraint_values) > 0
                else None,
        })
        return self._to_phys(self._build_fd_batch())

    # ---- 状态 ----

    def is_converged(self) -> bool: return self._converged
    def get_history(self) -> List[Dict[str, Any]]: return self._history

    def get_state(self) -> Dict[str, Any]:
        state: Dict[str, Any] = {
            "algorithm": "SGD",
            "design_ranges": self.design_ranges.tolist(),
            "hyperparams": self.hyperparams,
            "iteration": self._iteration,
            "converged": self._converged,
            "grad_norm": float(self._grad_norm),
        }
        if self._x is not None:
            state["x"] = self._x.tolist()
            state["velocity"] = self._velocity.tolist() if self._velocity is not None else None
            state["last_f"] = self._last_f
        return state

    def load_state(self, state: Dict[str, Any]) -> None:
        self._iteration = state.get("iteration", 0)
        self._converged = state.get("converged", False)
        self._grad_norm = state.get("grad_norm", np.inf)
        self._last_f = state.get("last_f", None)
        if "x" in state:
            self._x = np.array(state["x"], dtype=np.float64)
        if state.get("velocity") is not None:
            self._velocity = np.array(state["velocity"], dtype=np.float64)
