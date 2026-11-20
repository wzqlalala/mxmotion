# -*- coding: utf-8 -*-
"""差分进化（DE）。DE/rand/1/bin 策略。"""
from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple
import numpy as np
from mxsim_optim.base import OptimizerBase, MxsimValidationError, register_optimizer
from mxsim_optim.utils import unscale_from_unit, validate_positive_int, validate_probability

@register_optimizer("DE")
class DE(OptimizerBase):
    """差分进化。DE/rand/1/bin，支持离散变量。

    超参数
    ------
    pop_size : int, default=50
    n_objectives : int, default=1
    n_constraints : int, default=0
    max_generations : int, default=100
    F : float, default=0.8        缩放因子
    CR : float, default=0.9       交叉概率
    discrete_values : list, optional
    penalty_factor : float, default=1e6
    seed : int, optional
    """
    def __init__(self, design_ranges, hyperparams=None):
        super().__init__(design_ranges, hyperparams)
        self.pop_size = self.hyperparams.get("pop_size", 50)
        self.n_objectives = self.hyperparams.get("n_objectives", 1)
        self.n_constraints = self.hyperparams.get("n_constraints", 0)
        self.max_generations = self.hyperparams.get("max_generations", 100)
        self.F = self.hyperparams.get("F", 0.8)
        self.CR = self.hyperparams.get("CR", 0.9)
        self.penalty_factor = self.hyperparams.get("penalty_factor", 1e6)
        self.seed = self.hyperparams.get("seed", None)
        self._disc = self.hyperparams.get("discrete_values", None)
        validate_positive_int("pop_size", self.pop_size, min_val=4)
        self._rng = np.random.RandomState(self.seed)
        self._pop = None; self._gen = 0; self._conv = False

    def _to_phys(self, U): return unscale_from_unit(U, self.design_ranges)

    def _discretize(self, X):
        if self._disc is None: return X
        Xd = X.copy()
        for i, vals in enumerate(self._disc):
            if vals is not None:
                vv = np.asarray(vals).reshape(-1)
                lo, hi = self.design_ranges[i]
                vv_unit = (vv - lo) / (hi - lo + 1e-30)
                diffs = np.abs(Xd[:, i:i+1] - vv_unit.reshape(1, -1))
                best = np.argmin(diffs, axis=1)
                Xd[:, i] = vv_unit[best]
        return Xd

    def _fitness(self, F, G):
        # 多目标聚合为标量
        f = np.sum(np.atleast_2d(F), axis=1) if F.ndim > 1 else F.copy()
        if G is not None and self.n_constraints > 0:
            f += self.penalty_factor * np.sum(np.maximum(0, G)**2, axis=1) if G.ndim > 1 else self.penalty_factor * np.maximum(0, G)**2
        return f

    def initialize(self):
        self._pop = self._rng.uniform(0, 1, (self.pop_size, self.n_vars))
        self._pop = self._discretize(self._pop)
        self._gen = 0; self._conv = False; self._history = []
        return self._to_phys(self._pop)

    def update(self, objective_values, constraint_values=None):
        F = np.asarray(objective_values, dtype=np.float64).reshape(-1, max(1, self.n_objectives))
        G = constraint_values
        if G is not None: G = np.asarray(G, dtype=np.float64).reshape(-1, max(1, self.n_constraints))
        self._gen += 1
        fitness = self._fitness(F, G)

        # DE/rand/1/bin + 精英保留（批量评估模式下无法逐 trial 比较，用精英策略替代）
        # 保留精英个体 + 全部生成 trial 替换其余
        elite_n = max(2, self.pop_size // 10)
        elite_idx = np.argsort(fitness)[:elite_n]
        old_positions = self._to_phys(self._pop).tolist()

        new_pop = np.zeros_like(self._pop)
        for i in range(self.pop_size):
            cand = [j for j in range(self.pop_size) if j != i]
            a, b, c = self._pop[self._rng.choice(cand, 3, replace=False)]
            mutant = a + self.F * (b - c)
            cross = self._rng.uniform(0, 1, self.n_vars) < self.CR
            if not np.any(cross):
                cross[self._rng.randint(self.n_vars)] = True
            trial = np.where(cross, mutant, self._pop[i])
            trial = np.clip(trial, 0, 1)
            new_pop[i] = trial
        # 精英个体保留
        for idx in elite_idx:
            new_pop[idx] = self._pop[idx].copy()
        self._pop = self._discretize(new_pop)

        if self._gen >= self.max_generations: self._conv = True
        self._history.append({"generation": self._gen,
                              "positions": old_positions,  # 父代位置，匹配本次评估的 F
                              "objectives": F.tolist(),
                              "constraints": G.tolist() if G is not None else None})
        return self._to_phys(self._pop)

    def is_converged(self): return self._conv
    def get_history(self): return self._history
    def get_state(self):
        s = {"algorithm":"DE","design_ranges":self.design_ranges.tolist(),
             "hyperparams":self.hyperparams,"generation":self._gen,"converged":self._conv}
        if self._pop is not None: s["population"] = self._pop.tolist()
        return s
    def load_state(self, s):
        self._gen = s.get("generation",0); self._conv = s.get("converged",False)
        if "population" in s: self._pop = np.array(s["population"], dtype=np.float64)
