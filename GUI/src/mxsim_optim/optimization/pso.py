# -*- coding: utf-8 -*-
"""
粒子群优化（PSO）+ 多目标扩展（MOPSO）。

单目标：标准 PSO（惯性权重 + 个体/社会学习因子）。
多目标：MOPSO（外部存档 + Pareto 支配 + 拥挤距离选择）。
约束处理：单目标用罚函数法，多目标用约束支配原则。

参考文献：
    Kennedy & Eberhart (1995). Particle swarm optimization.
    Coello Coello & Lechuga (2002). MOPSO.
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


@register_optimizer("PSO")
class PSO(OptimizerBase):
    """粒子群优化算法。

    超参数
    ------
    pop_size : int, default=30
        种群（粒子群）大小。
    n_objectives : int, default=1
        目标函数个数。
    n_constraints : int, default=0
        约束个数（0=无约束）。
    w : float, default=0.7298
        惯性权重（Clerc & Kennedy 收缩系数）。
    c1 : float, default=1.49618
        个体认知加速系数。
    c2 : float, default=1.49618
        社会学习加速系数。
    max_iterations : int, default=100
        最大迭代次数。
    penalty_factor : float, default=1e6
        约束罚函数乘子（单目标用）。
    archive_size : int, default=50
        外部存档大小（MOPSO 用）。
    tol : float, default=1e-6
        收敛容差（速度范数）。
    seed : int, optional
        随机种子。
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        super().__init__(design_ranges, hyperparams)

        self.pop_size = self.hyperparams.get("pop_size", 30)
        self.n_objectives = self.hyperparams.get("n_objectives", 1)
        self.n_constraints = self.hyperparams.get("n_constraints", 0)
        self.max_iterations = self.hyperparams.get("max_iterations", 100)
        self.seed = self.hyperparams.get("seed", None)
        self.w = self.hyperparams.get("w", 0.7298)
        self.c1 = self.hyperparams.get("c1", 1.49618)
        self.c2 = self.hyperparams.get("c2", 1.49618)
        self.penalty_factor = self.hyperparams.get("penalty_factor", 1e6)
        self.archive_size = self.hyperparams.get("archive_size", 50)
        self.tol = self.hyperparams.get("tol", 1e-6)

        validate_positive_int("pop_size", self.pop_size, min_val=2)
        validate_positive_int("n_objectives", self.n_objectives)
        validate_positive_int("max_iterations", self.max_iterations)
        validate_positive_float("w", self.w)
        validate_positive_float("c1", self.c1)
        validate_positive_float("c2", self.c2)

        self._disc = self.hyperparams.get("discrete_values", None)
        self._rng = np.random.RandomState(self.seed)
        self._X: Optional[np.ndarray] = None       # 粒子群位置
        self._V: Optional[np.ndarray] = None       # 粒子群速度
        self._pbest: Optional[np.ndarray] = None   # 个体历史最优位置
        self._pbest_f: Optional[np.ndarray] = None # 个体历史最优目标值
        self._gbest: Optional[np.ndarray] = None   # 全局最优位置
        self._archive: List[np.ndarray] = []       # MOPSO 存档 [X, F]
        self._converged = False

    # ---- 辅助 ----

    def _to_phys(self, U: np.ndarray) -> np.ndarray:
        return unscale_from_unit(U, self.design_ranges)

    def _discretize(self, X):
        """将离散变量投影到最近允许值。"""
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

    @staticmethod
    def _dominates(a: np.ndarray, b: np.ndarray) -> bool:
        """Pareto 支配：a 在所有目标 <= b 且至少一个严格 <。"""
        return bool(np.all(a <= b) and np.any(a < b))

    @staticmethod
    def _crowding_distance(F: np.ndarray) -> np.ndarray:
        """Pareto 前沿 F 的拥挤距离。"""
        N, M = F.shape
        dist = np.zeros(N)
        if N <= 2:
            return np.ones(N)
        for m in range(M):
            idx = np.argsort(F[:, m])
            dist[idx[0]] = np.inf
            dist[idx[-1]] = np.inf
            r = F[idx[-1], m] - F[idx[0], m]
            if r > 1e-12:
                for i in range(1, N - 1):
                    dist[idx[i]] += (F[idx[i+1], m] - F[idx[i-1], m]) / r
        dist = np.where(np.isinf(dist), 1e10, dist)
        if dist.sum() < 1e-12:
            dist = np.ones(N)
        return dist

    # ---- 生命周期 ----

    def initialize(self) -> np.ndarray:
        """初始化粒子群，返回物理坐标下的初始种群。"""
        self._X = self._rng.uniform(0, 1, (self.pop_size, self.n_vars))
        self._X = self._discretize(self._X)
        self._V = self._rng.uniform(-0.1, 0.1, (self.pop_size, self.n_vars))
        self._pbest = self._X.copy()
        self._pbest_f = None
        self._gbest = None
        self._archive = []
        self._converged = False
        self._iteration = 0
        self._history = []
        return self._to_phys(self._X)

    def update(
        self,
        objective_values: np.ndarray,
        constraint_values: Optional[np.ndarray] = None,
    ) -> np.ndarray:
        """接收 C++ CAE 评估结果，返回下一代种群位置。

        Parameters
        ----------
        objective_values : shape (pop_size,) 或 (pop_size, n_obj)
        constraint_values : shape (pop_size, n_con) 或 None
        """
        F = np.asarray(objective_values, dtype=np.float64)
        if F.ndim == 1:
            F = F.reshape(-1, self.n_objectives)
        if F.shape[0] != self.pop_size:
            raise MxsimValidationError(
                f"期望 {self.pop_size} 个目标值，收到 {F.shape[0]}。")

        G = None
        if constraint_values is not None:
            G = np.asarray(constraint_values, dtype=np.float64)
            if G.ndim == 1:
                G = G.reshape(-1, max(1, self.n_constraints))
        if self.n_constraints > 0 and G is None:
            raise MxsimValidationError("需要约束值但未提供。")
        if G is not None and G.shape[0] != self.pop_size:
            raise MxsimValidationError(
                f"期望 {self.pop_size} 行约束值，收到 {G.shape[0]}。")

        # 先存旧位置（匹配本次传入的 F/G）再更新
        old_positions = self._to_phys(self._X).tolist()
        self._iteration += 1

        new_U = (self._update_single(F, G) if self.n_objectives == 1
                 else self._update_multi(F, G))

        self._history.append({
            "iteration": self._iteration,
            "positions": old_positions,            # 旧位置，与 objectives 匹配
            "objectives": F.tolist(),
            "constraints": G.tolist() if G is not None else None,
        })
        return self._to_phys(new_U)

    def _fitness(self, F: np.ndarray, G: Optional[np.ndarray]) -> np.ndarray:
        """带罚函数的标量适应度。"""
        fit = F[:, 0].copy().astype(np.float64)
        if G is not None and self.n_constraints > 0:
            fit += self.penalty_factor * np.sum(np.maximum(0, G)**2, axis=1)
        return fit

    def _update_single(self, F: np.ndarray, G: Optional[np.ndarray]) -> np.ndarray:
        """单目标 PSO 更新。"""
        fitness = self._fitness(F, G)

        if self._pbest_f is None:
            self._pbest_f = fitness.copy()
        else:
            better = fitness < self._pbest_f
            self._pbest[better] = self._X[better].copy()
            self._pbest_f[better] = fitness[better]

        self._gbest = self._pbest[int(np.argmin(self._pbest_f))].copy()

        r1 = self._rng.uniform(0, 1, (self.pop_size, self.n_vars))
        r2 = self._rng.uniform(0, 1, (self.pop_size, self.n_vars))

        self._V = (self.w * self._V
                   + self.c1 * r1 * (self._pbest - self._X)
                   + self.c2 * r2 * (self._gbest - self._X))
        self._V = np.clip(self._V, -0.2, 0.2)
        self._X = np.clip(self._X + self._V, 0, 1)
        self._X = self._discretize(self._X)

        if self._iteration >= self.max_iterations or np.max(np.abs(self._V)) < self.tol:
            self._converged = True
        return self._X

    def _update_multi(self, F: np.ndarray, G: Optional[np.ndarray]) -> np.ndarray:
        """MOPSO 多目标更新。"""
        # 约束违反量
        cv = (np.sum(np.maximum(0, G)**2, axis=1) if G is not None and self.n_constraints > 0
              else np.zeros(self.pop_size))

        # 更新个体最优
        if self._pbest_f is None:
            self._pbest_f = F.copy()
            self._pbest = self._X.copy()
            self._pbest_cv = cv.copy()
        else:
            for i in range(self.pop_size):
                curr_ok = cv[i] < 1e-8
                pbest_ok = self._pbest_cv[i] < 1e-8
                replace = False
                if curr_ok and pbest_ok:
                    if self._dominates(F[i], self._pbest_f[i]): replace = True
                elif curr_ok and not pbest_ok:
                    replace = True
                elif not curr_ok and not pbest_ok:
                    if cv[i] < self._pbest_cv[i]: replace = True
                if replace:
                    self._pbest[i] = self._X[i].copy()
                    self._pbest_f[i] = F[i].copy()
                    self._pbest_cv[i] = cv[i]

        # 更新存档
        feasible = cv < 1e-8
        fX, fF = self._X[feasible], F[feasible]
        if len(fF) > 0:
            if not self._archive:
                self._archive = [fX, fF]
            else:
                aX = np.vstack([self._archive[0], fX])
                aF = np.vstack([self._archive[1], fF])
                n_a = len(aF)
                dominated = np.zeros(n_a, bool)
                for p in range(n_a):
                    for q in range(n_a):
                        if p != q and self._dominates(aF[q], aF[p]):
                            dominated[p] = True; break
                keep = np.where(~dominated)[0]
                if len(keep) > self.archive_size:
                    cd = self._crowding_distance(aF[keep])
                    keep = keep[np.argsort(cd)[-self.archive_size:]]
                self._archive = [aX[keep], aF[keep]]

        # 从存档选领导者（轮盘赌，偏稀疏区域）
        if self._archive and len(self._archive[0]) > 0:
            aX, aF = self._archive
            cd = self._crowding_distance(aF)
            s = cd.sum()
            probs = cd / s if s > 1e-12 else np.ones(len(aX)) / len(aX)
            leaders = aX[self._rng.choice(len(aX), self.pop_size, p=probs)]
        else:
            leaders = self._pbest.copy()

        r1 = self._rng.uniform(0, 1, (self.pop_size, self.n_vars))
        r2 = self._rng.uniform(0, 1, (self.pop_size, self.n_vars))
        self._V = (self.w * self._V
                   + self.c1 * r1 * (self._pbest - self._X)
                   + self.c2 * r2 * (leaders - self._X))
        self._V = np.clip(self._V, -0.2, 0.2)
        self._X = np.clip(self._X + self._V, 0, 1)
        self._X = self._discretize(self._X)

        if self._iteration >= self.max_iterations:
            self._converged = True
        return self._X

    # ---- 状态 ----

    def is_converged(self) -> bool: return self._converged

    def get_history(self) -> List[Dict[str, Any]]: return self._history

    def get_state(self) -> Dict[str, Any]:
        state: Dict[str, Any] = {
            "algorithm": "PSO",
            "design_ranges": self.design_ranges.tolist(),
            "hyperparams": self.hyperparams,
            "iteration": self._iteration,
            "converged": self._converged,
        }
        if self._X is not None:
            state["X"] = self._X.tolist()
            state["V"] = self._V.tolist() if self._V is not None else None
            state["pbest"] = self._pbest.tolist()
            state["pbest_f"] = self._pbest_f.tolist() if self._pbest_f is not None else None
            state["gbest"] = self._gbest.tolist() if self._gbest is not None else None
            if hasattr(self, '_pbest_cv') and self._pbest_cv is not None:
                state["pbest_cv"] = self._pbest_cv.tolist()
        if self._archive:
            state["archive_X"] = self._archive[0].tolist()
            state["archive_F"] = self._archive[1].tolist()
        return state

    def load_state(self, state: Dict[str, Any]) -> None:
        self._iteration = state.get("iteration", 0)
        self._converged = state.get("converged", False)
        if "X" in state and state["X"] is not None:
            self._X = np.array(state["X"], dtype=np.float64)
            self._V = np.array(state.get("V", [[0]*self.n_vars]), dtype=np.float64)
            self._pbest = np.array(state["pbest"], dtype=np.float64)
            self._pbest_f = np.array(state.get("pbest_f", [[0]]), dtype=np.float64)
            if state.get("gbest") is not None:
                self._gbest = np.array(state["gbest"], dtype=np.float64)
            if state.get("pbest_cv") is not None:
                self._pbest_cv = np.array(state["pbest_cv"], dtype=np.float64)
        if "archive_X" in state:
            self._archive = [
                np.array(state["archive_X"], dtype=np.float64),
                np.array(state["archive_F"], dtype=np.float64),
            ]
