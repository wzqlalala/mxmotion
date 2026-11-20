# -*- coding: utf-8 -*-
"""遗传算法（GA）。锦标赛选择 + 模拟二进制交叉 + 多项式变异。"""
from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple
import numpy as np
from mxsim_optim.base import OptimizerBase, MxsimValidationError, register_optimizer
from mxsim_optim.utils import unscale_from_unit, validate_positive_int, validate_probability

@register_optimizer("GA")
class GA(OptimizerBase):
    """遗传算法。支持单/多目标，离散/连续变量。

    超参数
    ------
    pop_size : int, default=100
    n_objectives : int, default=1
    n_constraints : int, default=0
    max_generations : int, default=200
    crossover_prob : float, default=0.9
    mutation_prob : float, default=0.05
    tournament_size : int, default=3
    eta_c : float, default=10.0
    discrete_values : list, optional  每维离散取值列表，None=连续
    penalty_factor : float, default=1e6
    seed : int, optional
    """
    def __init__(self, design_ranges, hyperparams=None):
        super().__init__(design_ranges, hyperparams)
        self.pop_size = self.hyperparams.get("pop_size", 100)
        self.n_objectives = self.hyperparams.get("n_objectives", 1)
        self.n_constraints = self.hyperparams.get("n_constraints", 0)
        self.max_generations = self.hyperparams.get("max_generations", 200)
        self.crossover_prob = self.hyperparams.get("crossover_prob", 0.9)
        self.mutation_prob = self.hyperparams.get("mutation_prob", 0.05)
        self.tournament_size = self.hyperparams.get("tournament_size", 3)
        self.eta_c = self.hyperparams.get("eta_c", 10.0)
        self.penalty_factor = self.hyperparams.get("penalty_factor", 1e6)
        self.seed = self.hyperparams.get("seed", None)
        self._disc = self.hyperparams.get("discrete_values", None)  # list or None
        validate_positive_int("pop_size", self.pop_size, min_val=4)
        self._rng = np.random.RandomState(self.seed)
        self._pop = None; self._obj = None; self._gen = 0; self._conv = False

    def _to_phys(self, U): return unscale_from_unit(U, self.design_ranges)
    def _init_unit(self, n): return self._rng.uniform(0, 1, (n, self.n_vars))

    def _discretize(self, X):
        """将离散变量的值投影到最近允许值。"""
        if self._disc is None: return X
        Xd = X.copy()
        for i, vals in enumerate(self._disc):
            if vals is not None:
                vv = np.asarray(vals).reshape(-1)
                # 缩放到 [0,1] 范围
                lo, hi = self.design_ranges[i]
                vv_unit = (vv - lo) / (hi - lo + 1e-30)
                for k in range(Xd.shape[0]):
                    idx = np.argmin(np.abs(vv_unit - Xd[k, i]))
                    Xd[k, i] = vv_unit[idx]
        return Xd

    def _fitness(self, F, G):
        # 多目标聚合为标量：所有目标之和（单目标时退化为原值）
        f = np.sum(np.atleast_2d(F), axis=1) if F.ndim > 1 else F.copy()
        if G is not None and self.n_constraints > 0:
            f += self.penalty_factor * np.sum(np.maximum(0, G)**2, axis=1) if G.ndim > 1 else self.penalty_factor * np.maximum(0, G)**2
        return f

    def _tournament(self, fitness):
        idx = self._rng.choice(self.pop_size, self.tournament_size, replace=False)
        return idx[np.argmin(fitness[idx])]

    def _crossover(self, p1, p2):
        u = self._rng.uniform(0, 1, self.n_vars)
        beta = np.where(u <= 0.5, (2*u)**(1/(self.eta_c+1)), (1/(2*(1-u)))**(1/(self.eta_c+1)))
        c1 = np.clip(0.5*((1+beta)*p1 + (1-beta)*p2), 0, 1)
        c2 = np.clip(0.5*((1-beta)*p1 + (1+beta)*p2), 0, 1)
        return c1, c2

    def _mutate(self, x):
        r = self._rng.uniform(0, 1, self.n_vars)
        mask = r < self.mutation_prob
        if not np.any(mask): return x
        xm = x.copy()
        xm[mask] = self._rng.uniform(0, 1, size=np.sum(mask))
        return np.clip(xm, 0, 1)

    def initialize(self):
        self._pop = self._init_unit(self.pop_size)
        self._pop = self._discretize(self._pop)
        self._obj = None; self._gen = 0; self._conv = False; self._history = []
        return self._to_phys(self._pop)

    def update(self, objective_values, constraint_values=None):
        F = np.asarray(objective_values, dtype=np.float64).reshape(-1, max(1, self.n_objectives))
        G = constraint_values
        if G is not None: G = np.asarray(G, dtype=np.float64).reshape(-1, max(1, self.n_constraints))
        self._gen += 1
        fitness = self._fitness(F, G)

        # 精英保留 + 子代生成
        elite_n = max(2, self.pop_size // 10)
        elite_idx = np.argsort(fitness)[:elite_n]
        offspring = [self._pop[i].copy() for i in elite_idx]
        while len(offspring) < self.pop_size:
            t1 = self._tournament(fitness); t2 = self._tournament(fitness)
            p1, p2 = self._pop[t1], self._pop[t2]
            if self._rng.uniform() < self.crossover_prob:
                c1, c2 = self._crossover(p1, p2)
            else:
                c1, c2 = p1.copy(), p2.copy()
            offspring.append(self._discretize(self._mutate(c1).reshape(1,-1)).ravel())
            if len(offspring) < self.pop_size:
                offspring.append(self._discretize(self._mutate(c2).reshape(1,-1)).ravel())
        old_pos = self._to_phys(self._pop).tolist()  # 存旧种群位置

        self._pop = np.array(offspring[:self.pop_size])
        self._pop = self._discretize(self._pop)

        if self._gen >= self.max_generations: self._conv = True
        self._history.append({"generation": self._gen, "positions": old_pos,
                              "objectives": F.tolist(),
                              "constraints": G.tolist() if G is not None else None})
        return self._to_phys(self._pop)

    def is_converged(self): return self._conv
    def get_history(self): return self._history
    def get_state(self):
        s = {"algorithm":"GA","design_ranges":self.design_ranges.tolist(),
             "hyperparams":self.hyperparams,"generation":self._gen,"converged":self._conv}
        if self._pop is not None: s["population"] = self._pop.tolist()
        return s
    def load_state(self, s):
        self._gen = s.get("generation",0); self._conv = s.get("converged",False)
        if "population" in s: self._pop = np.array(s["population"], dtype=np.float64)
