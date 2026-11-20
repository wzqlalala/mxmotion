# -*- coding: utf-8 -*-
"""
基类与算法注册表。

提供：
    - AlgorithmRegistry  — 基于字典的工厂模式，注册/发现/创建算法
    - DOEBase             — 试验设计抽象基类
    - SensitivityBase     — 敏感性分析抽象基类
    - OptimizerBase       — 优化算法抽象基类
    - MxsimError          — 自定义异常体系

新增算法时只需用装饰器注册，无需修改调度逻辑。
"""

from __future__ import annotations

import abc
from typing import Any, Callable, Dict, List, Optional, Tuple

import numpy as np


# =============================================================================
# 自定义异常（pybind11 可自动转换为 C++ 可捕获的异常）
# =============================================================================

class MxsimError(Exception):
    """mxsim_optim 所有异常的统一基类。"""


class MxsimValidationError(MxsimError):
    """输入参数校验失败。"""


class MxsimRuntimeError(MxsimError):
    """运行时计算错误。"""


# =============================================================================
# 算法注册表（基于字典的工厂模式）
# =============================================================================

class AlgorithmRegistry:
    """字典驱动的算法注册表，按 DOE / Sensitivity / Optimizer 三类独立管理。

    用法::

        # 注册
        AlgorithmRegistry.register_doe('LHS', LHS)

        # 创建
        algo = AlgorithmRegistry.create_doe('LHS', **kwargs)

        # 列出已注册算法
        AlgorithmRegistry.list_doe()
    """

    _doe: Dict[str, type] = {}
    _sensitivity: Dict[str, type] = {}
    _optimizer: Dict[str, type] = {}

    # ---- DOE ----

    @classmethod
    def register_doe(cls, name: str, algo_cls: type) -> None:
        """注册一个 DOE 算法（名称不区分大小写）。"""
        key = name.upper()
        if key in cls._doe:
            raise MxsimError(f"DOE 算法 '{name}' 已注册。")
        cls._doe[key] = algo_cls

    @classmethod
    def create_doe(cls, name: str, **kwargs) -> "DOEBase":
        """实例化一个已注册的 DOE 算法。"""
        key = name.upper()
        if key not in cls._doe:
            raise MxsimValidationError(
                f"未知 DOE 算法 '{name}'，可用: {list(cls._doe.keys())}")
        return cls._doe[key](**kwargs)

    @classmethod
    def list_doe(cls) -> List[str]:
        """返回已注册 DOE 算法名称列表。"""
        return list(cls._doe.keys())

    # ---- Sensitivity ----

    @classmethod
    def register_sensitivity(cls, name: str, algo_cls: type) -> None:
        """注册一个敏感性分析方法。"""
        key = name.upper()
        if key in cls._sensitivity:
            raise MxsimError(f"敏感性算法 '{name}' 已注册。")
        cls._sensitivity[key] = algo_cls

    @classmethod
    def create_sensitivity(cls, name: str, **kwargs) -> "SensitivityBase":
        """实例化一个已注册的敏感性分析方法。"""
        key = name.upper()
        if key not in cls._sensitivity:
            raise MxsimValidationError(
                f"未知敏感性方法 '{name}'，可用: {list(cls._sensitivity.keys())}")
        return cls._sensitivity[key](**kwargs)

    @classmethod
    def list_sensitivity(cls) -> List[str]:
        """返回已注册敏感性方法名称列表。"""
        return list(cls._sensitivity.keys())

    # ---- Optimizer ----

    @classmethod
    def register_optimizer(cls, name: str, algo_cls: type) -> None:
        """注册一个优化算法。"""
        key = name.upper()
        if key in cls._optimizer:
            raise MxsimError(f"优化器 '{name}' 已注册。")
        cls._optimizer[key] = algo_cls

    @classmethod
    def create_optimizer(cls, name: str, **kwargs) -> "OptimizerBase":
        """实例化一个已注册的优化器。"""
        key = name.upper()
        if key not in cls._optimizer:
            raise MxsimValidationError(
                f"未知优化器 '{name}'，可用: {list(cls._optimizer.keys())}")
        return cls._optimizer[key](**kwargs)

    @classmethod
    def list_optimizers(cls) -> List[str]:
        """返回已注册优化器名称列表。"""
        return list(cls._optimizer.keys())


# =============================================================================
# 装饰器：简化注册
# =============================================================================

def register_doe(name: str) -> Callable:
    """类装饰器：将被装饰的类自动注册为 DOE 算法。"""
    def _decorator(cls: type) -> type:
        AlgorithmRegistry.register_doe(name, cls)
        return cls
    return _decorator


def register_sensitivity(name: str) -> Callable:
    """类装饰器：将被装饰的类自动注册为敏感性分析方法。"""
    def _decorator(cls: type) -> type:
        AlgorithmRegistry.register_sensitivity(name, cls)
        return cls
    return _decorator


def register_optimizer(name: str) -> Callable:
    """类装饰器：将被装饰的类自动注册为优化算法。"""
    def _decorator(cls: type) -> type:
        AlgorithmRegistry.register_optimizer(name, cls)
        return cls
    return _decorator


# =============================================================================
# 抽象基类
# =============================================================================

class DOEBase(abc.ABC):
    """试验设计基类。子类必须实现 ``generate()``。

    Parameters
    ----------
    design_ranges : list of [min, max]
        每个设计变量的取值范围，如 ``[[0,1], [-1,1]]``。
    n_samples : int
        采样点数。
    hyperparams : dict, optional
        算法专属超参数。
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        n_samples: int,
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        self.hyperparams = hyperparams or {}
        self._disc_vals = self.hyperparams.get("discrete_values", None)
        self._validate(design_ranges, n_samples)
        self.design_ranges = np.asarray(design_ranges, dtype=np.float64)
        self.n_samples = n_samples
        self.n_vars = len(design_ranges)

    def _validate(self, ranges, n):
        """校验 DOE 输入参数，离散变量自动填充 design_ranges。"""
        if not isinstance(ranges, list) or len(ranges) == 0:
            raise MxsimValidationError("design_ranges 必须是非空的 [min,max] 列表。")
        for i, r in enumerate(ranges):
            if self._disc_vals is not None and i < len(self._disc_vals) and self._disc_vals[i] is not None:
                # 离散变量：design_ranges 实际未用，但保留形式
                if not isinstance(r, (list, tuple)) or len(r) != 2:
                    raise MxsimValidationError(f"design_ranges[{i}] 离散变量也需 [placeholder_min, placeholder_max]。")
                continue
            if not isinstance(r, (list, tuple)) or len(r) != 2:
                raise MxsimValidationError(f"design_ranges[{i}] 必须是 [min,max] 对。")
            if r[0] >= r[1]:
                raise MxsimValidationError(f"design_ranges[{i}]: min({r[0]}) 必须 < max({r[1]})。")
        if not isinstance(n, int) or n < 2:
            raise MxsimValidationError(f"n_samples 必须是 >=2 的整数，得到 {n}。")

    def _apply_discrete(self, X: np.ndarray) -> np.ndarray:
        """将离散变量投影到最近允许值。"""
        if self._disc_vals is None:
            return X
        Xd = X.copy()
        for i, vals in enumerate(self._disc_vals):
            if vals is not None and i < Xd.shape[1]:
                vv = np.asarray(vals, dtype=np.float64).reshape(-1)
                for k in range(Xd.shape[0]):
                    idx = np.argmin(np.abs(vv - Xd[k, i]))
                    Xd[k, i] = vv[idx]
        return Xd

    @abc.abstractmethod
    def generate(self) -> np.ndarray:
        """返回 (n_samples, n_vars) 的采样点矩阵。"""
        ...


class SensitivityBase(abc.ABC):
    """敏感性分析基类。子类必须实现 ``generate_samples()`` 和 ``analyze()``。

    Parameters
    ----------
    design_ranges : list of [min, max]
        变量取值范围。
    n_samples : int
        基础采样数。
    hyperparams : dict, optional
        方法专属超参数。
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        n_samples: int,
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        self._validate(design_ranges, n_samples)
        self.design_ranges = np.asarray(design_ranges, dtype=np.float64)
        self.n_samples = n_samples
        self.hyperparams = hyperparams or {}
        self.n_vars = len(design_ranges)

    @staticmethod
    def _validate(ranges: List[Tuple[float, float]], n: int) -> None:
        """校验敏感性分析输入参数。"""
        if not isinstance(ranges, list) or len(ranges) == 0:
            raise MxsimValidationError("design_ranges 必须是非空列表。")
        for i, r in enumerate(ranges):
            if not isinstance(r, (list, tuple)) or len(r) != 2:
                raise MxsimValidationError(f"design_ranges[{i}] 必须是 [min,max]。")
            if r[0] >= r[1]:
                raise MxsimValidationError(f"design_ranges[{i}]: min >= max。")
        if not isinstance(n, int) or n < 2:
            raise MxsimValidationError(f"n_samples 必须是 >=2 的整数，得到 {n}。")

    @abc.abstractmethod
    def generate_samples(self) -> np.ndarray:
        """生成敏感性分析所需的采样矩阵。"""
        ...

    @abc.abstractmethod
    def analyze(self, Y: np.ndarray) -> Dict[str, Any]:
        """根据输出值 Y 执行敏感性分析，返回结构化结果。"""
        ...


class OptimizerBase(abc.ABC):
    """迭代优化算法基类。

    生命周期：
        initialize()                        → 返回初始种群
        update(obj_vals, con_vals)          → 接收评估结果，返回下一代种群
        ... 重复直到 is_converged() 为 True ...
        get_history() / get_state()         → 获取结果
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        self._validate(design_ranges)
        self.design_ranges = np.asarray(design_ranges, dtype=np.float64)
        self.n_vars = len(design_ranges)
        self.hyperparams = hyperparams or {}
        self._history: List[Dict[str, Any]] = []
        self._iteration = 0

    @staticmethod
    def _validate(ranges: List[Tuple[float, float]]) -> None:
        """校验优化器输入参数。"""
        if not isinstance(ranges, list) or len(ranges) == 0:
            raise MxsimValidationError("design_ranges 必须是非空的 [min,max] 列表。")
        for i, r in enumerate(ranges):
            if not isinstance(r, (list, tuple)) or len(r) != 2:
                raise MxsimValidationError(f"design_ranges[{i}] 必须是 [min,max]。")
            if r[0] >= r[1]:
                raise MxsimValidationError(f"design_ranges[{i}]: min >= max。")

    @abc.abstractmethod
    def initialize(self) -> np.ndarray:
        """返回初始种群，shape (pop_size, n_vars)。"""
        ...

    @abc.abstractmethod
    def update(
        self,
        objective_values: np.ndarray,
        constraint_values: Optional[np.ndarray] = None,
    ) -> np.ndarray:
        """接收 C++ CAE 的评估结果，返回下一代种群。"""
        ...

    @abc.abstractmethod
    def is_converged(self) -> bool:
        """是否满足终止条件。"""
        ...

    @abc.abstractmethod
    def get_history(self) -> List[Dict[str, Any]]:
        """返回所有迭代历史数据。"""
        ...

    @abc.abstractmethod
    def load_state(self, state: Dict[str, Any]) -> None:
        """从序列化字典恢复优化器状态。"""
        ...

    @abc.abstractmethod
    def get_state(self) -> Dict[str, Any]:
        """序列化当前状态到字典。"""
        ...
