# -*- coding: utf-8 -*-
"""
优化算法子模块。

提供：
    PSO    — 粒子群优化（单目标 + MOPSO 多目标，支持罚函数约束）
    NSGA2  — 非支配排序遗传算法 II（多目标，约束支配原则）
    SGD    — 随机梯度下降（有限差分梯度估计，支持投影/罚函数约束）

工厂与问题描述：
    OptimizerFactory  — 统一创建/恢复优化器
    ProblemSpec       — 声明目标方向（最小化/最大化）与约束形式（<=, >=, ==）
"""

from mxsim_optim.optimization.pso import PSO
from mxsim_optim.optimization.nsga2 import NSGA2
from mxsim_optim.optimization.sgd import SGD
from mxsim_optim.optimization.ga import GA
from mxsim_optim.optimization.de import DE
from mxsim_optim.optimization.aco import ACO
from mxsim_optim.optimization.optimizer import (
    OptimizerFactory,
    ProblemSpec,
    ObjectiveSpec,
    ConstraintSpec,
)
