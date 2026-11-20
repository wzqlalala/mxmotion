# -*- coding: utf-8 -*-
"""中心复合设计（CCD）。全因子/部分因子 + 轴点 + 中心点。"""
from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple
import numpy as np
from mxsim_optim.base import DOEBase, register_doe, MxsimValidationError
from mxsim_optim.utils import unscale_from_unit

@register_doe("CCD")
class CCDesign(DOEBase):
    """中心复合设计。= 角点（全因子） + 轴点 + 中心点，五水平响应面。

    超参数
    ------
    alpha : float, default=1.682
        轴点距离（=√d 可旋转，=1 面心）。
    center_points : int, default=1
        中心点重复次数。
    """
    def __init__(self, design_ranges, n_samples=None, hyperparams=None):
        hp = hyperparams or {}
        d = len(design_ranges)
        self._alpha = hp.get("alpha", np.sqrt(d))
        self._ctr = hp.get("center_points", 1)
        total = 2**d + 2*d + self._ctr
        super().__init__(design_ranges, n_samples or total, hp)

    def generate(self) -> np.ndarray:
        d = self.n_vars
        # 角点: 全因子 2^d（编码为 ±1）
        corners = np.array([
            [(b>>k)&1 for k in range(d)]
            for b in range(2**d)
        ], dtype=np.float64)

        # 轴点: 每个变量 ±alpha，其余为 0.5
        axes = np.zeros((2*d, d), dtype=np.float64)
        for i in range(d):
            axes[2*i, i]   = 0.5 + self._alpha / (2.0 * self._alpha + 2.0)
            axes[2*i+1, i] = 0.5 - self._alpha / (2.0 * self._alpha + 2.0)
        for i in range(d):
            for j in range(d):
                if i != j:
                    axes[2*i, j] = axes[2*i+1, j] = 0.5

        # 简化：用 ±1 编码 + 缩放
        U_corners = corners * 0.5 / np.sqrt(d) + 0.5 if d > 0 else corners * 0.5 + 0.5

        # 轴点编码（0~1 空间）
        U_axes = np.full((2*d, d), 0.5, dtype=np.float64)
        for i in range(d):
            alpha_norm = min(0.5, self._alpha / (2.0 * d))
            U_axes[2*i, i]   = 0.5 + alpha_norm
            U_axes[2*i+1, i] = 0.5 - alpha_norm

        # 中心点
        U_ctr = np.full((self._ctr, d), 0.5, dtype=np.float64)

        U = np.vstack([U_corners, U_axes, U_ctr])
        U = np.clip(U, 0.0, 1.0)
        return self._apply_discrete(unscale_from_unit(U, self.design_ranges))
