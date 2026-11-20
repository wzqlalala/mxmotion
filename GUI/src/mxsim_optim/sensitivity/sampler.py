# -*- coding: utf-8 -*-
"""
敏感性分析采样入口。

提供 ``generate_samples()`` 统一函数——创建对应的敏感性分析实例并返回采样矩阵。
``doe_method`` 参数会传递到各敏感性算法的内部采样策略中。
"""

from __future__ import annotations

from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from mxsim_optim.base import AlgorithmRegistry, MxsimValidationError


def generate_samples(
    method: str,
    design_ranges: List[Tuple[float, float]],
    n_samples: int,
    doe_method: str = "LHS",
    hyperparams: Optional[Dict[str, Any]] = None,
) -> np.ndarray:
    """为指定的敏感性分析方法生成采样矩阵。

    Parameters
    ----------
    method : str
        敏感性方法名：``"Morris"``、``"Sobol"`` 或 ``"HDMR"``。
    design_ranges : list of [min, max]
        设计变量取值范围。
    n_samples : int
        基础采样数（Morris 为轨迹数，Sobol 为基础样本数，HDMR 为切点数参考）。
    doe_method : str
        底层 DOE 采样策略（``"LHS"`` 或 ``"Optimal LHS"``）。
        会注入到各敏感性算法的 hyperparams 中，由其内部决定如何使用。
    hyperparams : dict, optional
        方法专属超参数。常用键：
        - Morris: ``trajectories`` (int), ``levels`` (int)
        - Sobol: ``order`` (int, 1 或 2)
        - HDMR: ``max_order`` (int), ``n_cuts`` (int)
        - seed (int): 随机种子

    Returns
    -------
    np.ndarray
        采样矩阵，形状依方法而定：
        - Morris: (trajectories × (n_vars+1), n_vars)
        - Sobol:  (n_samples × (2×n_vars+2), n_vars)
        - HDMR:   依 max_order 和 n_cuts 计算
    """
    hyperparams = dict(hyperparams or {})

    # 把 doe_method 注入 hyperparams，各算法内部自行决定是否使用
    if "doe_method" not in hyperparams:
        hyperparams["doe_method"] = doe_method

    method_key = method.upper()
    if method_key not in AlgorithmRegistry._sensitivity:
        raise MxsimValidationError(
            f"未知敏感性方法 '{method}'，可用: "
            f"{list(AlgorithmRegistry._sensitivity.keys())}")

    algo_cls = AlgorithmRegistry._sensitivity[method_key]
    algo = algo_cls(
        design_ranges=design_ranges,
        n_samples=n_samples,
        hyperparams=hyperparams,
    )
    return algo.generate_samples()
