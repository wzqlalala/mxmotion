# -*- coding: utf-8 -*-
"""Box-Behnken 设计。取边中点+中心点，避免角点。"""
from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple
import numpy as np
from mxsim_optim.base import DOEBase, register_doe, MxsimValidationError
from mxsim_optim.utils import unscale_from_unit

@register_doe("BB DESIGN")
class BBDesign(DOEBase):
    """Box-Behnken 设计。取超立方边中点（±1水平）和中心点，避免极端角点。

    超参数
    ------
    center_points : int, default=1
        中心点重复次数。
    """
    def __init__(self, design_ranges, n_samples=None, hyperparams=None):
        hp = hyperparams or {}
        self._ctr = hp.get("center_points", 1)
        d = len(design_ranges)
        n_pairs = d * (d - 1) // 2
        total = n_pairs * 4 + self._ctr
        super().__init__(design_ranges, n_samples or total, hp)
        if self.n_vars < 3:
            raise MxsimValidationError("Box-Behnken 需要至少 3 个变量")

    def generate(self) -> np.ndarray:
        d = self.n_vars
        pairs = [(i, j) for i in range(d) for j in range(i+1, d)]
        n_edge = len(pairs) * 4
        U = np.full((n_edge + self._ctr, d), 0.5, dtype=np.float64)
        row = 0
        for i, j in pairs:
            for vi, vj in [(0.0, 0.0), (0.0, 1.0), (1.0, 0.0), (1.0, 1.0)]:
                U[row, i] = vi
                U[row, j] = vj
                row += 1
        return self._apply_discrete(unscale_from_unit(U, self.design_ranges))
