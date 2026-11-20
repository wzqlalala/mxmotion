# -*- coding: utf-8 -*-
"""部分因子设计。取全因子的 1/2^k 子集，通过生成元定义别名结构。"""
from __future__ import annotations
from typing import Any, Dict, List, Optional, Tuple
import numpy as np
from mxsim_optim.base import DOEBase, register_doe, MxsimValidationError
from mxsim_optim.utils import unscale_from_unit, validate_positive_int

@register_doe("FRACTIONAL FACTORIAL")
class FractionalFactorial(DOEBase):
    """部分因子设计。取全因子的 1/2^k，用生成元定义。

    超参数
    ------
    fraction : int, default=1
        缩减因子 k，取 1/2^k。默认 1=半因子。
    generator : list[int], optional
        生成元列号（用于定义别名）。未指定时用最高阶交互作为生成元。
    """
    def __init__(self, design_ranges, n_samples=None, hyperparams=None):
        hp = hyperparams or {}
        d = len(design_ranges)
        self._frac = hp.get("fraction", 1)
        self._gen = hp.get("generator", None)
        total = 2**(d - self._frac)
        super().__init__(design_ranges, n_samples or total, hp)
        if 2**self._frac >= 2**d:
            raise MxsimValidationError("fraction 过大，无有效设计")

    def generate(self) -> np.ndarray:
        d = self.n_vars
        k = self._frac  # 缩减指数
        base_vars = d - k  # 基础因子数（全因子部分）
        n_runs = 2**base_vars

        # 基础因子的全因子
        U = np.zeros((n_runs, d))
        for i in range(base_vars):
            col = np.tile(np.repeat([0, 1], 2**(base_vars-1-i)), 2**i)
            U[:, i] = col

        # 生成元定义的附加因子（基础因子的交互乘积 mod 2）
        gen = self._gen or list(range(base_vars - k, base_vars))
        for idx, gi in enumerate(range(base_vars, d)):
            U[:, gi] = np.mod(U[:, gen[idx % len(gen)]] + U[:, (gen[idx]+1) % base_vars], 2)

        return self._apply_discrete(unscale_from_unit(U, self.design_ranges))
