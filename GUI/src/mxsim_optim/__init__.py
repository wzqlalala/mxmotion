# -*- coding: utf-8 -*-
"""
mxsim_optim — Python 优化模块，供 C++ CAE 求解器通过内嵌 Python 运行时调用。

可以通过 Cython 编译为 .pyd 二进制文件以保护源码。

子模块：
    doe          — 试验设计（LHS、最优 LHS）
    sensitivity  — 敏感性分析（Morris、Sobol、HDMR）
    optimization — 优化算法（PSO、NSGA-II、SGD）
"""

__version__ = "1.0.0"

from mxsim_optim.base import AlgorithmRegistry
from mxsim_optim import doe
from mxsim_optim import sensitivity
from mxsim_optim import optimization
