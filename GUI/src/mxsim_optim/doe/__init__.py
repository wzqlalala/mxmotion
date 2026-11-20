# -*- coding: utf-8 -*-
"""
DOE 试验设计子模块。

提供：
    LHS         — 标准拉丁超立方采样
    OptimalLHS  — 最优拉丁超立方采样（maximin 准则）

导入时自动通过 AlgorithmRegistry 注册。
"""

from mxsim_optim.doe.lhs import LHS
from mxsim_optim.doe.optimal_lhs import OptimalLHS
from mxsim_optim.doe.bbdesign import BBDesign
from mxsim_optim.doe.ccdesign import CCDesign
from mxsim_optim.doe.doptimal import DOptimal
from mxsim_optim.doe.full_factorial import FullFactorial
from mxsim_optim.doe.frac_factorial import FractionalFactorial
