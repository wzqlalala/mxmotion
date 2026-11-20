# -*- coding: utf-8 -*-
"""
Morris 基本效应法（Method of Elementary Effects）。

通过"一次一个因子"的轨迹设计，估计每个输入因子的基本效应，输出：
- μ：效应的均值
- μ*：效应绝对值的均值（综合重要性）
- σ：效应的标准差（表示非线性/交互作用程度）

参考文献：
    Morris, M. D. (1991). Factorial sampling plans for preliminary
    computational experiments. Technometrics, 33(2), 161–174.
"""

from __future__ import annotations

from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from mxsim_optim.base import (
    SensitivityBase,
    AlgorithmRegistry,
    MxsimValidationError,
    MxsimRuntimeError,
    register_sensitivity,
)
from mxsim_optim.utils import (
    _lhs_unit,
    unscale_from_unit,
    validate_positive_int,
)


@register_sensitivity("MORRIS")
class Morris(SensitivityBase):
    """Morris 敏感性分析。

    每条轨迹是一个 (n_vars+1)×n_vars 的矩阵，连续两行之间只有一个变量发生变化。
    总采样点数 = 轨迹数 × (n_vars + 1)。

    Parameters
    ----------
    n_samples : int
        期望的总采样点数。若未指定 ``trajectories``，轨迹数由
        ``n_samples // (n_vars + 1)`` 取整得到。

    超参数
    ------
    trajectories : int, optional
        Morris 轨迹数（优先于 n_samples 推断）。
    levels : int, default=4
        网格层数（当 ``doe_method`` 未设置时使用）。
    doe_method : str, optional
        底层 DOE 方法。设置后轨迹基准点由 DOE 采样，否则用网格层方法。
    seed : int, optional
        随机种子。
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        n_samples: int,
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        super().__init__(design_ranges, n_samples, hyperparams)
        # 轨迹数未指定时，由 n_samples // (n_vars+1) 推断
        n_vars = len(design_ranges)
        default_traj = max(2, n_samples // (n_vars + 1))
        self.trajectories = self.hyperparams.get("trajectories", default_traj)
        self.levels = self.hyperparams.get("levels", 4)
        self.seed = self.hyperparams.get("seed", None)
        self.doe_method = self.hyperparams.get("doe_method", None)

        validate_positive_int("trajectories", self.trajectories)
        validate_positive_int("levels", self.levels, min_val=2)

        # 内部状态
        self._sample_matrix: Optional[np.ndarray] = None
        self._delta: float = 0.0
        self._perms: List[np.ndarray] = []  # 每条轨迹的因子变化顺序

    # ------------------------------------------------------------------
    # 采样
    # ------------------------------------------------------------------

    def generate_samples(self) -> np.ndarray:
        """生成 Morris 轨迹设计矩阵。

        Returns
        -------
        np.ndarray, shape (trajectories × (n_vars+1), n_vars)
        """
        rng = np.random.RandomState(self.seed)
        n = self.n_vars
        r = self.trajectories

        total_rows = r * (n + 1)
        B = np.zeros((total_rows, n), dtype=np.float64)
        self._perms = []

        # 根据是否设置 doe_method 决定基准点采样方式
        if self.doe_method and self.doe_method.upper() != "NONE":
            base_points = self._gen_base_via_doe(rng)
            self._delta = 1.0 / (2.0 * max(float(r), 2.0))
        else:
            p = self.levels
            self._delta = p / (2.0 * (p - 1.0))
            grid = np.linspace(0.0, 1.0, p)
            base_points = grid[rng.randint(0, p, size=(r, n))]

        # 逐条构建轨迹
        for traj in range(r):
            x_star = base_points[traj, :]
            perm = rng.permutation(n)
            self._perms.append(perm)

            # 下三角矩阵 D*：第 i 行前 i 列为 1
            D_star = np.tril(np.ones((n + 1, n), dtype=np.float64), k=-1)

            # 按 perm 排序列，随机 ± 符号
            D = np.zeros((n + 1, n), dtype=np.float64)
            for j_idx, j in enumerate(perm):
                sign = 2.0 * rng.randint(0, 2) - 1.0
                for i in range(n + 1):
                    D[i, j] = sign * D_star[i, j_idx]

            # trajectory = J·x*^T + (Δ/2)·D，然后裁剪到 [0,1]
            x_star_mat = np.tile(x_star, (n + 1, 1))
            trajectory = np.clip(x_star_mat + (self._delta / 2.0) * D, 0.0, 1.0)

            B[traj * (n + 1):traj * (n + 1) + n + 1, :] = trajectory

        self._sample_matrix = unscale_from_unit(B, self.design_ranges)
        return self._sample_matrix

    def _gen_base_via_doe(self, rng: np.random.RandomState) -> np.ndarray:
        """通过指定的 DOE 方法在 [0,1]^n_vars 中生成基准点。"""
        unit_ranges = [[0.0, 1.0]] * self.n_vars
        seed = self.seed if self.seed is not None else rng.randint(0, 2**31)
        try:
            doe = AlgorithmRegistry.create_doe(
                self.doe_method,
                design_ranges=unit_ranges,
                n_samples=self.trajectories,
                hyperparams={"seed": seed},
            )
            return np.clip(doe.generate(), 0.0, 1.0)
        except MxsimValidationError as e:
            raise MxsimValidationError(
                f"Morris DOE 采样失败: '{self.doe_method}', "
                f"可用: {AlgorithmRegistry.list_doe()}, "
                f"原始错误: {e}")

    # ------------------------------------------------------------------
    # 分析
    # ------------------------------------------------------------------

    def analyze(self, Y: np.ndarray) -> Dict[str, Any]:
        """计算 Morris μ、μ* 和 σ。

        Parameters
        ----------
        Y : np.ndarray
            CAE 输出值，shape (total_rows,) 或 (total_rows, n_outputs)。

        Returns
        -------
        dict
            键: ``mu``, ``mu_star``, ``sigma``, ``elementary_effects``,
            ``n_vars``, ``trajectories``, ``method``。
        """
        if self._sample_matrix is None:
            raise MxsimRuntimeError("请先调用 generate_samples() 再调用 analyze()。")

        n = self.n_vars
        r = self.trajectories

        Y = np.atleast_2d(Y)
        if Y.shape[0] == 1:
            Y = Y.T
        if Y.shape[1] == 1:
            Y = Y.ravel()

        total_rows = r * (n + 1)
        if Y.shape[0] != total_rows:
            raise MxsimValidationError(
                f"Y 必须有 {total_rows} 行（={r} 条轨迹 × ({n}+1)），"
                f"实际 {Y.shape[0]} 行。")

        # 逐轨迹计算基本效应
        if Y.ndim == 1:
            n_out = 1
            ee = np.zeros((r, n))
            for traj in range(r):
                start = traj * (n + 1)
                perm = self._perms[traj]
                for step in range(n):
                    factor = perm[step]  # 这一步改了哪个因子
                    ee[traj, factor] = (
                        Y[start + step + 1] - Y[start + step]) / self._delta
            mu = np.mean(ee, axis=0)
            mu_star = np.mean(np.abs(ee), axis=0)
            sigma = np.std(ee, axis=0, ddof=1)
        else:
            n_out = Y.shape[1]
            ee = np.zeros((r, n, n_out))
            for traj in range(r):
                start = traj * (n + 1)
                perm = self._perms[traj]
                for step in range(n):
                    factor = perm[step]
                    ee[traj, factor, :] = (
                        Y[start + step + 1, :] - Y[start + step, :]) / self._delta
            mu = np.mean(ee, axis=0)
            mu_star = np.mean(np.abs(ee), axis=0)
            sigma = np.std(ee, axis=0, ddof=1)

        return {
            "mu": mu.tolist(),
            "mu_star": mu_star.tolist(),
            "sigma": sigma.tolist(),
            "elementary_effects": ee.tolist(),
            "n_vars": n,
            "trajectories": r,
            "method": "Morris",
        }
