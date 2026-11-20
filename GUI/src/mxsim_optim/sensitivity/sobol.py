# -*- coding: utf-8 -*-
"""
Sobol' 方差分解敏感性分析。

采用 Saltelli 2002/2010 估计器，用两套拟随机矩阵 A、B 及交叉矩阵
A_B^{(i)} 估计一阶（S1）和总效应（ST）指数。

参考文献：
    Saltelli, A. (2002). Making best use of model evaluations to compute
    sensitivity indices. Computer Physics Communications, 145, 280–297.
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


@register_sensitivity("SOBOL")
class Sobol(SensitivityBase):
    """Sobol' 方差敏感性分析。

    Parameters
    ----------
    n_samples : int
        Saltelli 采样的基础样本数 N。总采样点数 = n_samples × (n_vars + 2)。

    超参数
    ------
    order : int, default=1
        敏感性指数阶数（1 = S1+ST，2 = 也估计 S2）。
    doe_method : str, optional
        底层 DOE 方法，用于生成 A、B 矩阵。设置后替代内置 _lhs_unit。
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
        self.order = self.hyperparams.get("order", 1)
        self.seed = self.hyperparams.get("seed", None)
        self.doe_method = self.hyperparams.get("doe_method", None)
        validate_positive_int("order", self.order, min_val=1)
        if self.order > 2:
            raise MxsimValidationError(
                f"Sobol order={self.order} 超出实现范围（最大 2）。")

        self._sample_matrix: Optional[np.ndarray] = None
        self._A: Optional[np.ndarray] = None
        self._B: Optional[np.ndarray] = None

    # ------------------------------------------------------------------
    # 采样
    # ------------------------------------------------------------------

    def generate_samples(self) -> np.ndarray:
        """生成 Saltelli 采样矩阵。

        Returns
        -------
        np.ndarray, shape (N×(d+2), d)
            前 N 行=A，中间 N 行=B，后续 d 个 N 行块=A_B^{(i)}。
        """
        rng = np.random.RandomState(self.seed)
        N, d = self.n_samples, self.n_vars

        # 生成 A、B 矩阵
        if self.doe_method and self.doe_method.upper() != "NONE":
            A = self._sample_doe(rng, "A")
            B = self._sample_doe(rng, "B")
        else:
            A = _lhs_unit(N, d, seed=rng.randint(0, 2**31))
            B = _lhs_unit(N, d, seed=rng.randint(0, 2**31))

        # 拼合: [A; B; AB^(1); ...; AB^(d)]
        total = N * (d + 2)
        X_unit = np.zeros((total, d), dtype=np.float64)
        X_unit[0:N, :] = A
        X_unit[N:2*N, :] = B
        for i in range(d):
            block = A.copy()
            block[:, i] = B[:, i]
            X_unit[(2 + i) * N:(2 + i) * N + N, :] = block

        self._A = A
        self._B = B
        self._sample_matrix = unscale_from_unit(X_unit, self.design_ranges)
        return self._sample_matrix

    def _sample_doe(self, rng: np.random.RandomState, label: str) -> np.ndarray:
        """通过配置的 DOE 方法生成 (N,d) 矩阵。"""
        unit_ranges = [[0.0, 1.0]] * self.n_vars
        base_seed = self.seed if self.seed is not None else rng.randint(0, 2**31)
        # A/B 用固定偏移区分，保证可复现
        offset = 1 if label == "B" else 0
        seed = base_seed + offset
        try:
            doe = AlgorithmRegistry.create_doe(
                self.doe_method,
                design_ranges=unit_ranges,
                n_samples=self.n_samples,
                hyperparams={"seed": seed},
            )
            return np.clip(doe.generate(), 0.0, 1.0)
        except MxsimValidationError as e:
            raise MxsimValidationError(
                f"Sobol DOE 采样失败: '{self.doe_method}', "
                f"可用: {AlgorithmRegistry.list_doe()}, "
                f"原始错误: {e}")

    # ------------------------------------------------------------------
    # 分析
    # ------------------------------------------------------------------

    def analyze(self, Y: np.ndarray) -> Dict[str, Any]:
        """计算 Sobol' 一阶和总效应指数。

        Parameters
        ----------
        Y : np.ndarray
            CAE 输出，shape (N×(d+2),) 或 (N×(d+2), n_outputs)。

        Returns
        -------
        dict
            键: ``S1``, ``ST``, ``S2``（order=2 时）, ``method``,
            ``N``, ``n_vars``, ``n_outputs``。
        """
        if self._A is None or self._B is None:
            raise MxsimRuntimeError("请先调用 generate_samples() 再调用 analyze()。")

        N, d = self.n_samples, self.n_vars

        Y = np.atleast_2d(Y)
        if Y.shape[0] == 1:
            Y = Y.T
        if Y.shape[0] != N * (d + 2):
            raise MxsimValidationError(
                f"Y 必须有 {N*(d+2)} 行（=N×(d+2)={N}×{d+2}），"
                f"实际 {Y.shape[0]} 行。")

        n_out = Y.shape[1]
        results: Dict[str, Any] = {
            "S1": [], "ST": [],
            "method": "Sobol", "N": N, "n_vars": d, "n_outputs": n_out,
        }

        for out_idx in range(n_out):
            y = Y[:, out_idx]
            f_A = y[0:N]
            f_B = y[N:2*N]
            f_AB = np.zeros((d, N))
            for i in range(d):
                f_AB[i, :] = y[(2 + i) * N:(2 + i) * N + N]

            # 总方差
            var_Y = np.mean(f_A**2) - np.mean(f_A)**2

            S1_i = np.zeros(d)
            ST_i = np.zeros(d)
            if var_Y > 1e-30:
                for i in range(d):
                    S1_i[i] = np.mean(f_B * (f_AB[i, :] - f_A)) / var_Y
                    ST_i[i] = 0.5 * np.mean((f_A - f_AB[i, :])**2) / var_Y

            results["S1"].append(S1_i.tolist())
            results["ST"].append(ST_i.tolist())

        if n_out == 1:
            results["S1"] = results["S1"][0]
            results["ST"] = results["ST"][0]
        if self.order >= 2:
            results["S2"] = self._compute_s2(Y, N, d, n_out)

        return results

    def _compute_s2(self, Y: np.ndarray, N: int, d: int, n_out: int):
        """近似二阶 Sobol' 指数。"""
        s2 = []
        for out_idx in range(n_out):
            y = Y[:, out_idx]
            f_A = y[0:N]
            var_Y = np.mean(f_A**2) - np.mean(f_A)**2
            S2 = np.zeros((d, d))
            if var_Y > 1e-30:
                for i in range(d):
                    f_i = y[(2 + i) * N:(2 + i) * N + N]
                    for j in range(i + 1, d):
                        f_j = y[(2 + j) * N:(2 + j) * N + N]
                        V_ij = np.mean(f_i * f_j) - np.mean(f_A)**2
                        S2[i, j] = V_ij / var_Y - self._s1(y, i, N, var_Y)
                        S2[j, i] = S2[i, j]
            s2.append(S2.tolist())
        return s2[0] if n_out == 1 else s2

    @staticmethod
    def _s1(y: np.ndarray, i: int, N: int, var_Y: float) -> float:
        f_A = y[0:N]
        f_B = y[N:2*N]
        f_AB_i = y[(2 + i) * N:(2 + i) * N + N]
        return float(np.mean(f_B * (f_AB_i - f_A)) / var_Y)
