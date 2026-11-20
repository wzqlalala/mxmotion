# -*- coding: utf-8 -*-
"""蚁群优化（ACO）。适用于离散/组合优化，基于信息素路径选择。"""
from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple
import numpy as np
from mxsim_optim.base import OptimizerBase, MxsimValidationError, register_optimizer
from mxsim_optim.utils import unscale_from_unit, validate_positive_int

@register_optimizer("ACO")
class ACO(OptimizerBase):
    """蚁群优化。默认所有变量均为离散值，每维从 discrete_values 中选。

    超参数
    ------
    n_ants : int, default=30
    n_iterations : int, default=100
    evaporation : float, default=0.1   信息素蒸发率
    alpha : float, default=1.0         信息素重要度
    beta : float, default=2.0          启发因子重要度
    Q : float, default=100.0          信息素强度
    discrete_values : list of list     每维候选值列表（必填）
    n_constraints : int, default=0
    penalty_factor : float, default=1e6
    seed : int, optional
    """
    def __init__(self, design_ranges, hyperparams=None):
        super().__init__(design_ranges, hyperparams)
        self.n_ants = self.hyperparams.get("n_ants", 30)
        self.n_iterations = self.hyperparams.get("n_iterations", 100)
        self.evaporation = self.hyperparams.get("evaporation", 0.1)
        self.alpha = self.hyperparams.get("alpha", 1.0)
        self.beta = self.hyperparams.get("beta", 2.0)
        self.Q = self.hyperparams.get("Q", 100.0)
        self.n_constraints = self.hyperparams.get("n_constraints", 0)
        self.penalty_factor = self.hyperparams.get("penalty_factor", 1e6)
        self.seed = self.hyperparams.get("seed", None)
        self._disc = self.hyperparams.get("discrete_values", None)
        if self._disc is None:
            # 默认：每维等距取 10 个值
            self._disc = [np.linspace(r[0], r[1], 10).tolist() for r in design_ranges]
        self._n_opts = [len(v) for v in self._disc]
        self._pheromone = [np.ones(n) for n in self._n_opts]
        self._rng = np.random.RandomState(self.seed)
        self._best_f = np.inf; self._best_x = None; self._conv = False; self._iter = 0

    def _to_phys(self, U): return unscale_from_unit(U, self.design_ranges)

    def _decode(self, choices):
        """将每维选择的索引转为物理坐标。"""
        x = np.zeros((len(choices), self.n_vars))
        for i, vals in enumerate(self._disc):
            vv = np.asarray(vals)
            x[:, i] = vv[np.array([c[i] for c in choices]).astype(int)]
        return x

    def initialize(self):
        self._iter = 0; self._best_f = np.inf; self._best_x = None
        self._history = []
        # 随机选择
        choices = [tuple(self._rng.randint(0, self._n_opts[i], 1)[0] for i in range(self.n_vars)) for _ in range(self.n_ants)]
        self._pop = self._decode(choices)
        # 转为单位空间坐标存内部
        lo, hi = self.design_ranges[:, 0], self.design_ranges[:, 1]
        self._pop_unit = (self._pop - lo) / (hi - lo + 1e-30)
        return self._pop

    def update(self, objective_values, constraint_values=None):
        F = np.asarray(objective_values, dtype=np.float64)
        # 多目标聚合为标量
        F_agg = np.sum(F, axis=1) if F.ndim > 1 else F
        G = constraint_values
        if G is not None: G = np.asarray(G, dtype=np.float64).reshape(-1, max(1, self.n_constraints))
        self._iter += 1

        # 适应度
        fit = F_agg.copy()
        if G is not None and self.n_constraints > 0:
            fit += self.penalty_factor * np.sum(np.maximum(0, G)**2, axis=1) if G.ndim > 1 else self.penalty_factor * np.maximum(0, G)**2

        # 保存旧种群位置（与本次评估匹配）
        old_positions = self._pop.tolist()

        # 更新最优
        best_idx = np.argmin(fit)
        if fit[best_idx] < self._best_f:
            self._best_f = fit[best_idx]; self._best_x = self._pop[best_idx].copy()

        # 信息素更新：蒸发 + 沉积
        for dim in range(self.n_vars):
            self._pheromone[dim] *= (1.0 - self.evaporation)
            for ant in range(self.n_ants):
                idx = int(np.argmin(np.abs(np.asarray(self._disc[dim]) - self._pop[ant, dim])))
                self._pheromone[dim][idx] += self.Q / (fit[ant] + 1e-30)

        # 新一代蚂蚁
        choices = []
        for _ in range(self.n_ants):
            choice = []
            for dim in range(self.n_vars):
                eta = 1.0 / (np.bincount(
                    [int(np.argmin(np.abs(np.asarray(self._disc[dim]) - self._pop[j, dim])))
                     for j in range(len(self._pop))],
                    minlength=self._n_opts[dim]) + 1.0)
                p = (self._pheromone[dim] ** self.alpha) * (eta ** self.beta)
                if p.sum() < 1e-12: p = np.ones_like(p)
                p /= p.sum()
                choice.append(self._rng.choice(self._n_opts[dim], p=p))
            choices.append(tuple(choice))
        self._pop = self._decode(choices)

        lo, hi = self.design_ranges[:, 0], self.design_ranges[:, 1]
        self._pop_unit = (self._pop - lo) / (hi - lo + 1e-30)

        if self._iter >= self.n_iterations: self._conv = True
        self._history.append({"iteration": self._iter,
                              "positions": old_positions,
                              "objectives": F.tolist(),
                              "constraints": G.tolist() if G is not None else None})
        return self._pop

    def is_converged(self): return self._conv
    def get_history(self): return self._history
    def get_state(self):
        s = {"algorithm":"ACO","design_ranges":self.design_ranges.tolist(),
             "hyperparams":self.hyperparams,"iteration":self._iter,"converged":self._conv}
        if self._pop is not None: s["population"] = self._pop.tolist()
        s["best_f"] = self._best_f
        if self._best_x is not None: s["best_x"] = self._best_x.tolist()
        s["pheromone"] = [p.tolist() for p in self._pheromone]
        return s
    def load_state(self, s):
        self._iter = s.get("iteration",0); self._conv = s.get("converged",False)
        self._best_f = s.get("best_f", np.inf)
        if s.get("best_x") is not None: self._best_x = np.array(s["best_x"])
        if s.get("population") is not None: self._pop = np.array(s["population"])
        if s.get("pheromone") is not None:
            self._pheromone = [np.array(p) for p in s["pheromone"]]
