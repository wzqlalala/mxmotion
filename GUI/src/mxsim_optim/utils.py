# -*- coding: utf-8 -*-
"""
工具函数。

- 输入校验
- 单位超立方 [0,1]^d 与物理空间之间的坐标缩放
- 拉丁超立方采样辅助函数
- 最小距离计算（用于最优 LHS 的 maximin 准则）
"""

from __future__ import annotations

from typing import List, Optional, Tuple

import numpy as np

from mxsim_optim.base import MxsimValidationError


# =============================================================================
# 坐标缩放：[0,1]^d ↔ 物理空间
# =============================================================================

def scale_to_unit(X: np.ndarray, design_ranges: np.ndarray) -> np.ndarray:
    """将物理空间坐标缩放到 [0,1]^d。

    Parameters
    ----------
    X : shape (N, d)
        物理空间中的点。
    design_ranges : shape (d, 2)
        第 0 列为各维下界，第 1 列为上界。

    Returns
    -------
    shape (N, d)，值域 [0,1]。
    """
    lo = design_ranges[:, 0]
    hi = design_ranges[:, 1]
    return (X - lo) / (hi - lo)


def unscale_from_unit(U: np.ndarray, design_ranges: np.ndarray) -> np.ndarray:
    """将 [0,1]^d 坐标还原到物理空间。

    Parameters
    ----------
    U : shape (N, d)
        单位超立方中的点。
    design_ranges : shape (d, 2)

    Returns
    -------
    shape (N, d)，物理空间坐标。
    """
    lo = design_ranges[:, 0]
    hi = design_ranges[:, 1]
    return U * (hi - lo) + lo


# =============================================================================
# 输入校验
# =============================================================================

def validate_positive_int(name: str, value: int, min_val: int = 1) -> None:
    """校验 *value* 是否为 >= *min_val* 的整数。"""
    if not isinstance(value, int) or value < min_val:
        raise MxsimValidationError(
            f"'{name}' 必须是 >= {min_val} 的整数，得到 {value}。")


def validate_positive_float(name: str, value: float) -> None:
    """校验 *value* 是否为正数。"""
    if not isinstance(value, (int, float)) or value <= 0:
        raise MxsimValidationError(
            f"'{name}' 必须是正数，得到 {value}。")


def validate_probability(name: str, value: float) -> None:
    """校验 *value* 是否在 [0,1] 区间。"""
    if not isinstance(value, (int, float)) or value < 0 or value > 1:
        raise MxsimValidationError(
            f"'{name}' 必须在 [0,1] 范围内，得到 {value}。")


# =============================================================================
# 距离计算（最优 LHS 的 maximin 准则）
# =============================================================================

def pairwise_min_distance(X: np.ndarray) -> float:
    """计算点集 *X* 中所有点对的最小欧氏距离（适用于 N ≤ 1000）。"""
    diff = X[:, np.newaxis, :] - X[np.newaxis, :, :]
    dist = np.sqrt(np.sum(diff ** 2, axis=-1))
    np.fill_diagonal(dist, np.inf)
    return float(np.min(dist))


def columnwise_pairwise_min_distance(X: np.ndarray) -> float:
    """逐列计算最小距离（内存友好，适用于较大 N）。"""
    n = X.shape[0]
    sq = np.full((n, n), np.inf, dtype=np.float64)
    for k in range(X.shape[1]):
        diff = X[:, k, np.newaxis] - X[np.newaxis, :, k]
        sq += diff ** 2
    np.fill_diagonal(sq, np.inf)
    return float(np.sqrt(np.min(sq)))


# =============================================================================
# 拉丁超立方采样（单位超立方）
# =============================================================================

def _lhs_unit(n_samples: int, n_vars: int, seed: Optional[int] = None) -> np.ndarray:
    """在 [0,1]^d 中生成拉丁超立方样本。

    每维分成 N 个等概率区间，每个区间内随机放置一个点。

    Parameters
    ----------
    n_samples : int
        样本数。
    n_vars : int
        变量维度。
    seed : int or None
        随机种子。

    Returns
    -------
    shape (n_samples, n_vars)
    """
    rng = np.random.RandomState(seed)
    H = np.zeros((n_samples, n_vars), dtype=np.float64)
    for j in range(n_vars):
        H[:, j] = rng.permutation(n_samples)
    H = (H + rng.uniform(size=(n_samples, n_vars))) / n_samples
    return H
