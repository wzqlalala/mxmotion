# -*- coding: utf-8 -*-
"""
NSGA-II — 非支配排序遗传算法 II。

支持多目标优化，采用快速非支配排序 + 拥挤距离选择，
约束处理使用约束支配原则。

遗传算子：锦标赛选择 + 模拟二进制交叉（SBX）+ 多项式变异。

参考文献：
    Deb, K., et al. (2002). A fast and elitist multiobjective genetic
    algorithm: NSGA-II. IEEE TEC, 6(2), 182–197.
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
    validate_probability,
)


@register_optimizer("NSGA2")
class NSGA2(OptimizerBase):
    """NSGA-II 多目标遗传算法。

    超参数
    ------
    pop_size : int, default=100
        种群大小（必须为偶数）。
    n_objectives : int, default=2
        目标个数。
    n_constraints : int, default=0
        约束个数。
    max_generations : int, default=200
        最大代数。
    crossover_prob : float, default=0.9
        交叉概率。
    mutation_prob : float, default=1/n_vars
        逐变量变异概率。
    eta_c : float, default=20.0
        SBX 分布指数。
    eta_m : float, default=20.0
        多项式变异分布指数。
    seed : int, optional
        随机种子。
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        super().__init__(design_ranges, hyperparams)

        self.pop_size = self.hyperparams.get("pop_size", 100)
        self.n_objectives = self.hyperparams.get("n_objectives", 2)
        self.n_constraints = self.hyperparams.get("n_constraints", 0)
        self.max_generations = self.hyperparams.get("max_generations", 200)
        self.crossover_prob = self.hyperparams.get("crossover_prob", 0.9)
        self.eta_c = self.hyperparams.get("eta_c", 20.0)
        self.eta_m = self.hyperparams.get("eta_m", 20.0)
        self.seed = self.hyperparams.get("seed", None)

        self.mutation_prob = self.hyperparams.get("mutation_prob", None)
        if self.mutation_prob is None:
            self.mutation_prob = 1.0 / max(self.n_vars, 1)

        validate_positive_int("pop_size", self.pop_size, min_val=4)
        if self.pop_size % 2 != 0:
            raise MxsimValidationError("pop_size 必须为偶数。")
        validate_positive_int("n_objectives", self.n_objectives)
        validate_positive_int("max_generations", self.max_generations)
        validate_probability("crossover_prob", self.crossover_prob)
        validate_probability("mutation_prob", self.mutation_prob)

        self._disc = self.hyperparams.get("discrete_values", None)
        self._rng = np.random.RandomState(self.seed)
        self._population: Optional[np.ndarray] = None
        self._objectives: Optional[np.ndarray] = None
        self._constraints: Optional[np.ndarray] = None
        self._generation = 0
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

    # ---- SBX 交叉 ----

    def _sbx(self, p1: np.ndarray, p2: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        u = self._rng.uniform(0, 1, size=self.n_vars)
        beta = np.where(u <= 0.5,
                        (2*u)**(1/(self.eta_c+1)),
                        (1/(2*(1-u)))**(1/(self.eta_c+1)))
        c1 = np.clip(0.5*((1+beta)*p1 + (1-beta)*p2), 0, 1)
        c2 = np.clip(0.5*((1-beta)*p1 + (1+beta)*p2), 0, 1)
        return c1, c2

    # ---- 多项式变异 ----

    def _mutate(self, x: np.ndarray) -> np.ndarray:
        r = self._rng.uniform(0, 1, size=self.n_vars)
        mask = r < self.mutation_prob
        if not np.any(mask):
            return x
        delta = self._rng.uniform(0, 1, size=self.n_vars)
        dq = np.where(delta < 0.5,
                      (2*delta)**(1/(self.eta_m+1)) - 1,
                      1 - (2*(1-delta))**(1/(self.eta_m+1)))
        xm = x.copy()
        xm[mask] = np.clip(x[mask] + dq[mask], 0, 1)
        return xm

    # ---- 约束支配 ----

    def _constraint_dominates(self, f1: np.ndarray, cv1: float,
                               f2: np.ndarray, cv2: float) -> bool:
        """约束支配：可行解支配不可行解；同可行→Pareto支配；同不可行→比约束违反量。"""
        ok1, ok2 = cv1 < 1e-8, cv2 < 1e-8
        if ok1 and not ok2: return True
        if not ok1 and ok2: return False
        if not ok1 and not ok2: return cv1 < cv2
        return bool(np.all(f1 <= f2) and np.any(f1 < f2))

    # ---- 快速非支配排序 ----

    def _fast_non_dominated_sort(self, F: np.ndarray, CV: np.ndarray) -> List[List[int]]:
        N = len(F)
        S = [[] for _ in range(N)]
        n = np.zeros(N, dtype=int)
        fronts: List[List[int]] = [[]]

        for i in range(N):
            for j in range(N):
                if i == j: continue
                if self._constraint_dominates(F[i], CV[i], F[j], CV[j]):
                    S[i].append(j)
                elif self._constraint_dominates(F[j], CV[j], F[i], CV[i]):
                    n[i] += 1
            if n[i] == 0:
                fronts[0].append(i)

        idx = 0
        while len(fronts[idx]) > 0:
            nxt = []
            for i in fronts[idx]:
                for j in S[i]:
                    n[j] -= 1
                    if n[j] == 0:
                        nxt.append(j)
            idx += 1
            if nxt:
                fronts.append(nxt)
            else:
                break
        return fronts

    @staticmethod
    def _crowding_distance(F: np.ndarray) -> np.ndarray:
        N, M = F.shape
        dist = np.zeros(N)
        if N <= 2:
            return np.ones(N)
        for m in range(M):
            idx = np.argsort(F[:, m])
            dist[idx[0]] = dist[idx[-1]] = np.inf
            r = F[idx[-1], m] - F[idx[0], m]
            if r > 1e-12:
                for i in range(1, N - 1):
                    dist[idx[i]] += (F[idx[i+1], m] - F[idx[i-1], m]) / r
        dist = np.where(np.isinf(dist), 1e10, dist)
        if dist.sum() < 1e-12:
            dist = np.ones(N)
        return dist

    # ---- 锦标赛选择 ----

    def _tournament(self, F: np.ndarray, cv: np.ndarray) -> int:
        i, j = self._rng.choice(self.pop_size, size=2, replace=False)
        if self._constraint_dominates(F[i], cv[i], F[j], cv[j]): return i
        if self._constraint_dominates(F[j], cv[j], F[i], cv[i]): return j
        cd = self._crowding_distance(F)
        return i if cd[i] >= cd[j] else j

    # ---- 生命周期 ----

    def initialize(self) -> np.ndarray:
        self._population = self._discretize(self._rng.uniform(0, 1, (self.pop_size, self.n_vars)))
        self._objectives = None
        self._constraints = None
        self._generation = 0
        self._converged = False
        self._history = []
        return self._to_phys(self._population)

    def update(
        self,
        objective_values: np.ndarray,
        constraint_values: Optional[np.ndarray] = None,
    ) -> np.ndarray:
        F = np.asarray(objective_values, dtype=np.float64)
        if F.ndim == 1:
            F = F.reshape(-1, self.n_objectives)
        if F.shape[0] != self.pop_size:
            raise MxsimValidationError(
                f"期望 {self.pop_size} 行目标值，收到 {F.shape[0]}。")

        if self.n_constraints > 0:
            if constraint_values is None:
                raise MxsimValidationError("需要约束值但未提供。")
            G = np.asarray(constraint_values, dtype=np.float64)
            if G.ndim == 1:
                G = G.reshape(-1, max(1, self.n_constraints))
        else:
            G = np.zeros((self.pop_size, 1))

        self._objectives = F
        self._constraints = G
        self._generation += 1

        cv = (np.sum(np.maximum(0, G)**2, axis=1) if self.n_constraints > 0
              else np.zeros(self.pop_size))

        old_positions = self._to_phys(self._population).tolist()

        # 产生子代
        offspring = np.zeros_like(self._population)
        for i in range(0, self.pop_size, 2):
            t1 = self._tournament(F, cv)
            t2 = self._tournament(F, cv)
            p1, p2 = self._population[t1], self._population[t2]
            if self._rng.uniform() < self.crossover_prob:
                c1, c2 = self._sbx(p1, p2)
            else:
                c1, c2 = p1.copy(), p2.copy()
            offspring[i] = self._discretize(self._mutate(c1))
            offspring[i+1] = self._discretize(self._mutate(c2))

        # NSGA-II (µ+λ) 环境选择：合并父代+子代 → 非支配排序 → 拥挤距离截断
        combined = np.vstack([self._population, offspring])
        # 为合并集构造临时的 F/G/cv（子代的 F/G 在下一轮才由 CAE 评估，此处用未知占位）
        # 注：标准 (µ+λ) 需子代目标值，在批量评估接口下无法即时获得；
        # 退化为"精英保留+拥挤距离"混合：父代精英 + 子代新颖性
        # 实际策略：保留父代 Pareto 前沿个体 + 子代补充
        parent_pareto = self._pareto_indices(F, cv)
        elite_n = min(len(parent_pareto), self.pop_size // 4)
        if elite_n > 0:
            elite = self._population[parent_pareto[:elite_n]]
            # 从子代中随机选择填充剩余
            n_remaining = self.pop_size - elite_n
            selected = self._rng.choice(len(offspring), n_remaining, replace=False)
            next_pop = np.vstack([elite, offspring[selected]])
        else:
            next_pop = offspring

        self._population = self._discretize(next_pop)

        if self._generation >= self.max_generations:
            self._converged = True

        self._history.append({
            "generation": self._generation,
            "positions": old_positions,
            "objectives": F.tolist(),
            "constraints": G.tolist() if self.n_constraints > 0 and G is not None else None,
            "constraint_violation": cv.tolist(),
            "pareto_front": self._pareto_indices(F, cv),
        })
        return self._to_phys(self._population)  # 返回离散化后的种群

    def _pareto_indices(self, F: np.ndarray, cv: np.ndarray) -> List[int]:
        fronts = self._fast_non_dominated_sort(F, cv)
        return [int(i) for i in fronts[0] if cv[i] < 1e-8]

    # ---- 状态 ----

    def is_converged(self) -> bool: return self._converged
    def get_history(self) -> List[Dict[str, Any]]: return self._history

    def get_state(self) -> Dict[str, Any]:
        state: Dict[str, Any] = {
            "algorithm": "NSGA2",
            "design_ranges": self.design_ranges.tolist(),
            "hyperparams": self.hyperparams,
            "generation": self._generation,
            "converged": self._converged,
        }
        if self._population is not None:
            state["population"] = self._population.tolist()
        if self._objectives is not None:
            state["objectives"] = self._objectives.tolist()
        if self._constraints is not None:
            state["constraints"] = self._constraints.tolist()
        return state

    def load_state(self, state: Dict[str, Any]) -> None:
        self._generation = state.get("generation", 0)
        self._converged = state.get("converged", False)
        if "population" in state:
            self._population = np.array(state["population"], dtype=np.float64)
        if "objectives" in state:
            self._objectives = np.array(state["objectives"], dtype=np.float64)
        if "constraints" in state:
            self._constraints = np.array(state["constraints"], dtype=np.float64)
