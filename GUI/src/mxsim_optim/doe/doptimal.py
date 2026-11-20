# -*- coding: utf-8 -*-
"""D-最优设计。从候选集中选点最大化 |X^T X|。"""
from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple
import numpy as np
from mxsim_optim.base import DOEBase, register_doe
from mxsim_optim.utils import unscale_from_unit, validate_positive_int

@register_doe("D-OPTIMAL")
class DOptimal(DOEBase):
    """D-最优设计。从大量候选 LHS 样本中贪心选点，最大化信息矩阵行列式。

    超参数
    ------
    n_candidates : int, default=500
        候选集大小（从中选择 n_samples 个点）。
    n_restarts : int, default=5
        随机重启次数。
    seed : int, optional
        随机种子。
    """
    def __init__(self, design_ranges, n_samples, hyperparams=None):
        super().__init__(design_ranges, n_samples, hyperparams)
        self._ncand = self.hyperparams.get("n_candidates", 500)
        self._restarts = self.hyperparams.get("n_restarts", 5)
        self._seed = self.hyperparams.get("seed", None)

    def generate(self) -> np.ndarray:
        rng = np.random.RandomState(self._seed)
        N, d = self.n_samples, self.n_vars
        best_det, best_idx = -np.inf, None

        for _ in range(self._restarts):
            # 生成候选集（LHS 在 [0,1]^d）
            cand = rng.uniform(0, 1, (self._ncand, d))
            # 选第一个点：离中心最远
            idx = [int(np.argmax(np.sum((cand - 0.5)**2, axis=1)))]
            for _ in range(N - 1):
                # 计算每个候选点加入后的 |X^TX|
                dets = np.zeros(self._ncand)
                for k in range(self._ncand):
                    if k in idx:
                        dets[k] = -np.inf; continue
                    X = np.vstack([cand[idx], cand[k:k+1]])
                    Xc = X - X.mean(axis=0)
                    dets[k] = np.linalg.det(Xc.T @ Xc + np.eye(d)*1e-6)
                idx.append(int(np.argmax(dets)))
            X = cand[idx]
            Xc = X - X.mean(axis=0)
            det = np.linalg.det(Xc.T @ Xc + np.eye(d)*1e-6)
            if det > best_det:
                best_det, best_idx = det, idx

        U = np.clip(cand[best_idx], 0, 1)
        return self._apply_discrete(unscale_from_unit(U, self.design_ranges))
