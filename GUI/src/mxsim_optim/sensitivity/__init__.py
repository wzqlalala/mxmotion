# -*- coding: utf-8 -*-
"""
敏感性分析子模块。

提供：
    Morris  — Morris 基本效应法（μ、σ）
    Sobol   — Sobol' 方差分解法（一阶 S1、总效应 ST）
    HDMR    — 高维模型表示（Cut-HDMR 分量函数与方差贡献）

顶层便捷函数：
    generate_samples(method, design_ranges, n_samples, doe_method, **hyperparams)
"""

from mxsim_optim.sensitivity.morris import Morris
from mxsim_optim.sensitivity.sobol import Sobol
from mxsim_optim.sensitivity.hdmr import HDMR
from mxsim_optim.sensitivity.sampler import generate_samples
