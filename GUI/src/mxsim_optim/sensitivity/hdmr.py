# -*- coding: utf-8 -*-
"""
高维模型表示（HDMR）— Cut-HDMR 变体。

Cut-HDMR 将多元函数 f(x) 围绕参考点 x̄ 展开为分量函数之和：

    f(x) = f₀ + Σ fᵢ(xᵢ) + Σ fᵢⱼ(xᵢ,xⱼ) + ...

本模块实现最多 2 阶 Cut-HDMR，输出分量函数及其方差贡献。

参考文献：
    Rabitz, H., & Aliş, Ö. F. (1999). General foundations of high‐dimensional
    model representations. Journal of Mathematical Chemistry, 25, 197–233.
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
from mxsim_optim.utils import unscale_from_unit, validate_positive_int


@register_sensitivity("HDMR")
class HDMR(SensitivityBase):
    """Cut-HDMR 敏感性分析。

    围绕参考点展开，在一维/二维切面上评估分量函数，报告各分量方差占比。

    Parameters
    ----------
    n_samples : int
        每维切点数（若 hyperparams 中未指定 ``n_cuts``）。
        总采样点数 ≈ 1 + n_vars×n_cuts（max_order=1）或在此基础上加二阶项。

    超参数
    ------
    max_order : int, default=2
        最大展开阶数（1 或 2）。
    n_cuts : int, default=n_samples
        每维切点数（奇数推荐，能把中点包括进去）。优先于 ``n_samples``。
    reference_point : list of float, optional
        切中心（物理坐标）。默认取各范围中点。
    doe_method : str, optional
        设置后切点位置用 DOE 采样代替等距采样。
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
        self.max_order = self.hyperparams.get("max_order", 2)
        self.n_cuts = self.hyperparams.get("n_cuts", n_samples)
        self.seed = self.hyperparams.get("seed", None)
        self.doe_method = self.hyperparams.get("doe_method", None)

        validate_positive_int("max_order", self.max_order, min_val=1)
        validate_positive_int("n_cuts", self.n_cuts, min_val=3)
        if self.max_order > 2:
            raise MxsimValidationError(
                f"HDMR max_order={self.max_order} 超出实现范围（最大 2）。")

        # 参考点（切中心）
        ref = self.hyperparams.get("reference_point", None)
        if ref is not None:
            if len(ref) != self.n_vars:
                raise MxsimValidationError(
                    f"reference_point 长度必须为 {self.n_vars}。")
            self.reference_point = np.asarray(ref, dtype=np.float64)
        else:
            self.reference_point = self.design_ranges.mean(axis=1)

        self._sample_matrix: Optional[np.ndarray] = None
        self._row_map: Dict[str, slice] = {}  # 各分量对应的行切片

    # ------------------------------------------------------------------
    # 采样
    # ------------------------------------------------------------------

    def generate_samples(self) -> np.ndarray:
        """生成 Cut-HDMR 采样矩阵。

        Returns
        -------
        np.ndarray
            包含参考点 + 一/二阶切面的全部评估点。
        """
        d, m = self.n_vars, self.n_cuts
        rng = np.random.RandomState(self.seed)

        # 切点位置（[0,1] 中）
        if self.doe_method and self.doe_method.upper() != "NONE":
            cuts_unit = self._gen_cuts_via_doe(rng, m)
        else:
            cuts_unit = np.linspace(0.0, 1.0, m)

        # 参考点在单位空间中的位置
        lo = self.design_ranges[:, 0]
        hi = self.design_ranges[:, 1]
        ref_unit = (self.reference_point - lo) / (hi - lo)

        blocks: List[np.ndarray] = []
        offset = 0

        # -- f0: 参考点 --
        blocks.append(ref_unit.reshape(1, d))
        self._row_map["f0"] = slice(offset, offset + 1)
        offset += 1

        # -- fi: 一阶切面 --
        for i in range(d):
            pts = np.tile(ref_unit, (m, 1))
            pts[:, i] = cuts_unit
            blocks.append(pts)
            self._row_map[f"f{i}"] = slice(offset, offset + m)
            offset += m

        # -- fij: 二阶切面 --
        if self.max_order >= 2:
            for i in range(d):
                for j in range(i + 1, d):
                    pts = np.tile(ref_unit, (m * m, 1))
                    ci, cj = np.meshgrid(cuts_unit, cuts_unit, indexing="ij")
                    pts[:, i] = ci.ravel()
                    pts[:, j] = cj.ravel()
                    blocks.append(pts)
                    self._row_map[f"f{i}{j}"] = slice(offset, offset + m * m)
                    offset += m * m

        X_unit = np.vstack(blocks)
        self._sample_matrix = unscale_from_unit(X_unit, self.design_ranges)
        return self._sample_matrix

    def _gen_cuts_via_doe(self, rng: np.random.RandomState, n_pts: int) -> np.ndarray:
        """用配置的 DOE 方法生成 [0,1] 中的一维切点位置（排序后返回）。"""
        seed = self.seed if self.seed is not None else rng.randint(0, 2**31)
        try:
            doe = AlgorithmRegistry.create_doe(
                self.doe_method,
                design_ranges=[[0.0, 1.0]],
                n_samples=n_pts,
                hyperparams={"seed": seed},
            )
            pts = np.clip(doe.generate().ravel(), 0.0, 1.0)
            pts.sort()
            return pts
        except MxsimValidationError as e:
            raise MxsimValidationError(
                f"HDMR DOE 采样失败: '{self.doe_method}', "
                f"可用: {AlgorithmRegistry.list_doe()}, "
                f"原始错误: {e}")

    # ------------------------------------------------------------------
    # 分析
    # ------------------------------------------------------------------

    def analyze(self, Y: np.ndarray) -> Dict[str, Any]:
        """计算 HDMR 分量函数与方差贡献。

        Parameters
        ----------
        Y : np.ndarray
            CAE 输出，shape (n_total,) 或 (n_total, n_outputs)。

        Returns
        -------
        dict
            键: ``f0``, ``fi``, ``fij``, ``fi_variance_fraction``,
            ``fij_variance_fraction``, ``total_variance``, ``reference_point``,
            ``method``。
        """
        if self._sample_matrix is None:
            raise MxsimRuntimeError("请先调用 generate_samples() 再调用 analyze()。")

        Y = np.atleast_2d(Y)
        if Y.shape[0] == 1:
            Y = Y.T
        n_out, d, m = Y.shape[1], self.n_vars, self.n_cuts

        results_all = []
        for out_idx in range(n_out):
            y = Y[:, out_idx]

            f0_val = float(np.mean(y[self._row_map["f0"]]))

            # 一阶分量
            fi, fi_vars = [], []
            for i in range(d):
                yi = y[self._row_map[f"f{i}"]]
                fi.append((yi - f0_val).tolist())
                fi_vars.append(float(np.var(yi, ddof=1)))

            # 二阶分量
            fij, fij_vars = [], []
            if self.max_order >= 2:
                for i in range(d):
                    for j in range(i + 1, d):
                        yij = y[self._row_map[f"f{i}{j}"]]
                        # 减去低阶贡献
                        fi_arr = np.array(fi[i])
                        fj_arr = np.array(fi[j])
                        mean_i = np.tile(fi_arr, m).reshape(m, m)
                        mean_j = np.tile(fj_arr, m).reshape(m, m).T
                        yij_grid = yij.reshape(m, m)
                        comp = yij_grid - f0_val - mean_i - mean_j
                        fij.append(comp.ravel().tolist())
                        fij_vars.append(float(np.var(yij_grid, ddof=1)))

            total_var = float(np.var(y, ddof=1)) if len(y) > 1 else 0.0
            if total_var > 1e-30:
                fi_fracs = [v / total_var for v in fi_vars]
                fij_fracs = [v / total_var for v in fij_vars]
            else:
                fi_fracs = [0.0] * len(fi_vars)
                fij_fracs = [0.0] * len(fij_vars)

            results_all.append({
                "f0": f0_val,
                "fi": fi, "fij": fij if self.max_order >= 2 else [],
                "fi_variance_fraction": fi_fracs,
                "fij_variance_fraction": fij_fracs,
                "total_variance": total_var,
            })

        result: Dict[str, Any] = {
            "method": "HDMR", "max_order": self.max_order,
            "n_vars": d, "n_cuts": m, "n_outputs": n_out,
            "reference_point": self.reference_point.tolist(),
        }
        if n_out == 1:
            result.update(results_all[0])
        else:
            result["per_output"] = results_all
        return result
