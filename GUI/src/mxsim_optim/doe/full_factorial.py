# -*- coding: utf-8 -*-
"""全因子设计。每维取指定层数，遍历所有组合。"""
from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple
import numpy as np
from mxsim_optim.base import DOEBase, register_doe, MxsimValidationError
from mxsim_optim.utils import unscale_from_unit

@register_doe("FULL FACTORIAL")
class FullFactorial(DOEBase):
    """全因子设计。每维 levels 个等距水平，生成 levels^d 个样本。

    超参数
    ------
    levels : int or list[int], default=3
        每维水平数。传 int 时所有维相同，传 list 时逐维指定。
    """
    def __init__(self, design_ranges, n_samples=None, hyperparams=None):
        hp = hyperparams or {}
        lev = hp.get("levels", 3)
        if isinstance(lev, int):
            self._levels = [lev] * len(design_ranges)
        else:
            if len(lev) != len(design_ranges):
                raise MxsimValidationError(f"levels 长度须等于变量数 {len(design_ranges)}")
            self._levels = list(lev)
        super().__init__(design_ranges, n_samples or int(np.prod(self._levels)), hp)

    def generate(self) -> np.ndarray:
        grids = [np.linspace(0, 1, L) for L in self._levels]
        mesh = np.meshgrid(*grids, indexing="ij")
        U = np.column_stack([m.ravel() for m in mesh])
        return self._apply_discrete(unscale_from_unit(U, self.design_ranges))
